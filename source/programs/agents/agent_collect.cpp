#include "support/configCosmos.h"
#include "agent/agentclass.h"

static Agent *agent;
static socket_channel rcvchan;
static uint16_t dbport = 11111;
//static int32_t mmsi;

int main(int argc, char *argv[])
{
    int32_t iretn = 0;

    agent = new Agent("server", "tsunami", 15.);

    if ((iretn = agent->wait()) < 0) {
        agent->debug_error.Printf("%16.10f %s Failed to start Agent %s on Node %s Dated %s : %s\n",currentmjd(), mjd2iso8601(currentmjd()).c_str(), agent->getAgent().c_str(), agent->getNode().c_str(), utc2iso8601(data_ctime(argv[0])).c_str(), cosmos_error_string(iretn).c_str());
        exit(iretn);
    } else {
        agent->debug_error.Printf("%16.10f %s Started Agent %s on Node %s Dated %s\n",currentmjd(), mjd2iso8601(currentmjd()).c_str(), agent->getAgent().c_str(), agent->getNode().c_str(), utc2iso8601(data_ctime(argv[0])).c_str());
    }

    // Open socket for receiving NMEA messages. Should be broadcast so should not need address
    if ((iretn=socket_open(&rcvchan, NetworkType::UDP, "", dbport, SOCKET_LISTEN, SOCKET_BLOCKING, AGENTRCVTIMEO)) != 0)
    {
        agent->shutdown();
        printf("Could not open incoming socket for forwarding: %d\n", iretn);
        exit (1);
    }

    string message;
    while (agent->running())
    {
        // Collect next set of GPS messages
        iretn = socket_recvfrom(rcvchan, message, 200);
        if (iretn > 0)
        {
            string cmd = "/usr/bin/mongosh --quiet tsunami";
            FILE *stream = popen(cmd.c_str(), "w");
            fprintf(stream, "db.data.insert(%s)\n", message.c_str());
            fprintf(stream, "exit\n");
            fclose(stream);
        }
    }
    agent->shutdown();
}
