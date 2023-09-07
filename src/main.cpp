
#include "json.hpp"
#include "joystick.hh"

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <librdkafka/rdkafka.h>
#include <cstdlib>
#include <signal.h>

using namespace std;

bool g_exit = false;

// Define the function to be called when ctrl-c (SIGINT) is sent to process
void signal_callback_handler(int signum)
{
    cout << "Caught signal " << signum << endl;
    // Terminate program
    g_exit = true;
}

using namespace nlohmann;

/**
 * Very simply application that reads in joystick messages
 * and pushes them out kafka
 */
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Must give configuration file" << std::endl;
        std::cout << "Usage: ./joystick2kafka config.json" << std::endl;
        exit(1);
    }
    std::string fileName(argv[1]);
    std::ifstream f(fileName);
    json data = json::parse(f);

    std::cout << "Loading configuration\n"
              << data.dump(2) << std::endl;

    std::string kafkaIp = data["kafkaIp"].get<std::string>();
    std::string topic = data["kafkaTopic"].get<std::string>();
    std::string joystickDev = data["joystick"].get<std::string>();
    uint16_t kafkaPort = data["kafkaPort"].get<uint16_t>();

    // Register signal and signal handler
    signal(SIGINT, signal_callback_handler);

    // Create an instance of Joystick
    Joystick joystick(joystickDev);

    // Ensure that it was found and that we can use it
    if (!joystick.isFound())
    {
        std::cout << "Failed to open joystick " << joystickDev << std::endl;
        exit(1);
    }

    // sets up kafka
    char hostname[128];
    char errstr[512];

    rd_kafka_conf_t *conf = rd_kafka_conf_new();

    if (gethostname(hostname, sizeof(hostname)))
    {
        fprintf(stderr, "%% Failed to lookup hostname\n");
        exit(1);
    }

    if (rd_kafka_conf_set(conf, "client.id", hostname,
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        fprintf(stderr, "%% %s\n", errstr);
        exit(1);
    }

    string bootstrapServer = kafkaIp + ":" + to_string(kafkaPort);
    if (rd_kafka_conf_set(conf, "bootstrap.servers", bootstrapServer.c_str(),
                          errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        fprintf(stderr, "%% %s\n", errstr);
        exit(1);
    }

    rd_kafka_topic_conf_t *topic_conf = rd_kafka_topic_conf_new();
    if (rd_kafka_topic_conf_set(topic_conf, "acks", "all",
                                errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK)
    {
        fprintf(stderr, "%% %s\n", errstr);
        exit(1);
    }

    /* Create Kafka producer handle */
    rd_kafka_t *rk;
    if (!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf,
                            errstr, sizeof(errstr))))
    {
        fprintf(stderr, "%% Failed to create new producer: %s\n", errstr);
        exit(1);
    }
    rd_kafka_topic_t *rkt = rd_kafka_topic_new(rk, topic.c_str(), topic_conf);

    json packet;
    while (!g_exit)
    {
        // Restrict rate
        usleep(1000);

        // Attempt to sample an event from the joystick
        JoystickEvent event;
        if (joystick.sample(&event))
        {
            packet["number"] = event.number;
            packet["value"] = event.value;
            packet["time"] = event.time;
            if (event.isButton())
            {
                packet["isAxis"] = false;
                printf("Button %u is %s\n",
                       event.number,
                       event.value == 0 ? "up" : "down");
            }
            else if (event.isAxis())
            {
                packet["isAxis"] = true;
                printf("Axis %u is at position %d\n", event.number, event.value);
            }

            std::string payload = packet.dump();
            cout << "Sending Packet: " << payload << endl;
            if (rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA,
                                 RD_KAFKA_MSG_F_COPY,
                                 (void*)payload.c_str(), payload.size(),
                                 NULL, 0,
                                 NULL) == -1)
            {
                fprintf(stderr, "%% Failed to produce to topic %s: %s\n",
                        topic.c_str(), rd_kafka_err2str(rd_kafka_last_error()));
            }
        }
    }

    // clean up
    rd_kafka_topic_destroy(rkt);
    rd_kafka_destroy(rk);

    return 0;
}