#include "support/configCosmos.h"
#include "agent/agentclass.h"
#include "support/jsonclass.h"

#define GPSPORT 10000


static Agent *agent;
static socket_channel rcvchan;
static socket_channel sendchan;
string gpsip;
uint16_t gpsport;
string dbip;
uint16_t dbport;
int32_t mmsi;

int main(int argc, char *argv[])
{
    int32_t iretn;

    // Load configuration file
    ifstream cs(data_name_path(agent->nodeName, "", "") + "config.json");
    string jstr;
    if (!cs.is_open())
    {
        printf("Could not open configuration file: %d\n", iretn);
        exit (1);
    }

    if (!getline(cs, jstr))
    {
        printf("Could not read configuration file: %d\n", -errno);
        exit (1);
    }

    // Extract configuration values from loaded JSON string
    Json jconfig;
    jconfig.extract_contents(jstr);
    gpsip = jconfig.ObjectContents.at("gpsip").svalue;
    gpsport = jconfig.ObjectContents.at("gpsport").nvalue;
    dbip = jconfig.ObjectContents.at("dbip").svalue;
    dbport = jconfig.ObjectContents.at("dbport").nvalue;
    mmsi = jconfig.ObjectContents.at("mmsi").nvalue;


    // Start Agent, using MMSI as node name
    agent = new Agent("mmsi"+to_string(mmsi), "tsunami", 15.);

    if ((iretn = agent->wait()) < 0) {
        fprintf(agent->get_debug_fd(), "%16.10f %s Failed to start Agent %s on Node %s Dated %s : %s\n",currentmjd(), mjd2iso8601(currentmjd()).c_str(), agent->getAgent().c_str(), agent->getNode().c_str(), utc2iso8601(data_ctime(argv[0])).c_str(), cosmos_error_string(iretn).c_str());
        exit(iretn);
    } else {
        fprintf(agent->get_debug_fd(), "%16.10f %s Started Agent %s on Node %s Dated %s\n",currentmjd(), mjd2iso8601(currentmjd()).c_str(), agent->getAgent().c_str(), agent->getNode().c_str(), utc2iso8601(data_ctime(argv[0])).c_str());
    }

    // Open socket for receiving NMEA messages. Should be broadcast so should not need address
    if ((iretn=socket_open(&rcvchan, NetworkType::UDP, "", gpsport, SOCKET_LISTEN, SOCKET_BLOCKING, AGENTRCVTIMEO)) != 0)
    {
        agent->shutdown();
        printf("Could not open incoming socket for forwarding: %d\n", iretn);
        exit (1);
    }

    // Open socket for sending JSON messages
    if ((iretn=socket_open(&sendchan, NetworkType::UDP, dbip.c_str(), dbport, SOCKET_TALK, SOCKET_BLOCKING, AGENTRCVTIMEO)) != 0)
    {
        agent->shutdown();
        printf("Could not open incoming socket for forwarding: %d\n", iretn);
        exit (1);
    }

    string message;
    while (agent->running())
    {
        // Collect next GPS message
        iretn = socket_recvfrom(rcvchan, message, 100);
        if (iretn > 0)
        {
            // Valid message received
            if (message.find("GGA") != string::npos)
            {

            }
        }
    }
}
