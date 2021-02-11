#include "support/configCosmos.h"
#include "agent/agentclass.h"
#include "support/jsonclass.h"

#define GPSPORT 10000


static Agent *agent;
static socket_channel rcvchan;
static socket_channel sendchan;
static string gpsip;
static uint16_t gpsport;
static string dbip;
static uint16_t dbport;
static int32_t mmsi;

int main(int argc, char *argv[])
{
    int32_t iretn = 0;
    double utcgga = 0.;
    double utcgst = 0.;
    double lat;
    char latc;
    double lon;
    char lonc;
    uint16_t gpsqi;
    uint16_t svnum;
    double hdop;
    double height;
    char heightu;
    double geoidsep;
    char geoidsepc;
    double age;
    double refid;
    uint16_t csum;
    char tid;
    double rms;
    double errormaj;
    double errormin;
    double erroratt;
    double latsig;
    double lonsig;
    double heightsig;

    // Load configuration file
//    ifstream cs(data_name_path(agent->nodeName, "", "") + "config.json");
    ifstream cs(string(argv[1]) + "/config.json");
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

    printf("The gpsip is: %s\nThe gpsport is: %d\nThe dbip is: %s\nThe dbport is: %d\nThe mmsi is: %d\n",gpsip.c_str(), gpsport, dbip.c_str(), dbport, mmsi);

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
    uint8_t ccsum;
    while (agent->running())
    {
        // Collect next GPS message
        iretn = socket_recvfrom(rcvchan, message, 200);
        if (iretn > 0)
        {printf("valid message received\n%s\n", message.c_str());
            // Valid message received
            if (message.find("GGA") != string::npos)
            {printf("GGA Message\n");
                iretn = sscanf(message.c_str(), "$GPGGA,%lf,%lf,%c,%lf,%c,%hu,%hu,%lf,%lf,%c,%lf,%c,%lf,%lf*%hu",
                               &utcgga,
                               &lat,
                               &latc,
                               &lon,
                               &lonc,
                               &gpsqi,
                               &svnum,
                               &hdop,
                               &height,
                               &heightu,
                               &geoidsep,
                               &geoidsepc,
                               &age,
                               &refid,
                               &csum);
                ccsum = 0;
                for (uint16_t i=1; i<message.size()-3; ++i)
                {
                    ccsum ^= message[i];
                }
                if (csum != ccsum)
                {
                    utcgga = 0.;
                }
            }

            if (message.find("GST") != string::npos)
            {printf("GST Message\n");
                iretn = sscanf(message.c_str(), "$G%cGST,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf*%hu",
                               &tid,
                               &utcgst,
                               &rms,
                               &errormaj,
                               &errormin,
                               &erroratt,
                               &latsig,
                               &lonsig,
                               &heightsig,
                               &csum);
                ccsum = 0;
                for (uint16_t i=1; i<message.size()-3; ++i)
                {
                    ccsum ^= message[i];
                }
                if (csum != ccsum)
                {
                    utcgst = 0.;
                }
            }
        }
    }
}
