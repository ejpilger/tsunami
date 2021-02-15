#include "support/configCosmos.h"
#include "agent/agentclass.h"
#include "support/jsonclass.h"
#include "support/jsonobject.h"


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
    struct gpsstruc
    {
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
    };

    deque <gpsstruc> gpsdata;

    // Load configuration file
    string nodes = get_cosmosnodes();
    if (nodes.empty())
    {
        printf("No ~/cosmos/nodes folder\n");
        exit (NODE_ERROR_ROOTDIR);
    }

    ifstream cs(nodes + "/config.json");
    string jstr;
    if (!cs.is_open())
    {
        printf("Could not open configuration file: %d\n", iretn);
        exit (iretn);
    }

    if (!getline(cs, jstr))
    {
        printf("Could not read configuration file: %d\n", -errno);
        exit (-errno);
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
    gpsstruc nextgps;
    ElapsedTime et;
    string gstpath;
    double gstmjd = currentmjd();
    string ggapath;
    double ggamjd = currentmjd();
    while (agent->running())
    {
        // Collect next set of GPS messages
        iretn = socket_recvfrom(rcvchan, message, 200);
        if (iretn > 0)
        {
            printf("valid message received\n%s\n", message.c_str());
            // Valid message received
            if (message.find("GGA") != string::npos)
            {
                ggapath = log_write(agent->nodeName, agent->agentName, ggamjd, "", "gga", message);
                printf("GGA Message\n");
                iretn = sscanf(message.c_str(), "$GPGGA,%lf,%lf,%c,%lf,%c,%hu,%hu,%lf,%lf,%c,%lf,%c,%lf,%lf*%hu",
                               &nextgps.utcgga,
                               &nextgps.lat,
                               &nextgps.latc,
                               &nextgps.lon,
                               &nextgps.lonc,
                               &nextgps.gpsqi,
                               &nextgps.svnum,
                               &nextgps.hdop,
                               &nextgps.height,
                               &nextgps.heightu,
                               &nextgps.geoidsep,
                               &nextgps.geoidsepc,
                               &nextgps.age,
                               &nextgps.refid,
                               &nextgps.csum);
                ccsum = 0;
                for (uint16_t i=1; i<message.size()-3; ++i)
                {
                    ccsum ^= message[i];
                }
                if (nextgps.csum != ccsum)
                {
                    nextgps.utcgga = 0.;
                }
            }

            if (message.find("GST") != string::npos)
            {
                log_write(agent->nodeName, agent->agentName, gstmjd, "", "gst", message);

                printf("GST Message\n");
                iretn = sscanf(message.c_str(), "$G%cGST,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf*%hu",
                               &nextgps.tid,
                               &nextgps.utcgst,
                               &nextgps.rms,
                               &nextgps.errormaj,
                               &nextgps.errormin,
                               &nextgps.erroratt,
                               &nextgps.latsig,
                               &nextgps.lonsig,
                               &nextgps.heightsig,
                               &nextgps.csum);
                ccsum = 0;
                for (uint16_t i=1; i<message.size()-3; ++i)
                {
                    ccsum ^= message[i];
                }
                if (nextgps.csum != ccsum)
                {
                    nextgps.utcgst = 0.;
                }
            }

            // If we have collected both records then push it to the FIFO
            if (nextgps.utcgga > 0. && nextgps.utcgst > 0.)
            {
                gpsdata.push_back(nextgps);
                if (gpsdata.size() > 120)
                {
                    gpsdata.pop_front();
                }
                nextgps.utcgga = 0.;
                nextgps.utcgst = 0.;
            }
        }

        // Every minute, average whatever is in the FIFO. up to 120 seconds
        if (et.split() > 60.)
        {
            gpsstruc tempgps;
            if (gpsdata.size())
            {
                for (gpsstruc gps : gpsdata)
                {
                    tempgps.lat += gps.lat;
                    tempgps.lon += gps.lon;
                    tempgps.height += gps.height;
                    tempgps.latsig += gps.latsig * gps.latsig;
                    tempgps.lonsig += gps.lonsig * gps.lonsig;
                    tempgps.heightsig += gps.heightsig * gps.heightsig;
                    tempgps.utcgga += gps.utcgga;
                    tempgps.utcgst += gps.utcgst;
                }
                tempgps.lat /= gpsdata.size();
                tempgps.lon /= gpsdata.size();
                tempgps.height /= gpsdata.size();
                tempgps.utcgga /= gpsdata.size();
                tempgps.utcgst /= gpsdata.size();
                tempgps.latsig = sqrt(tempgps.latsig);
                tempgps.lonsig = sqrt(tempgps.lonsig);
                tempgps.heightsig = sqrt(tempgps.heightsig);
            }
            JSONObject jobject;
            jobject.addElement("utcgga", tempgps.utcgga);
            jobject.addElement("utcgst", tempgps.utcgst);
            jobject.addElement("lat", tempgps.lat);
            jobject.addElement("lon", tempgps.lon);
            jobject.addElement("height", tempgps.height);
            jobject.addElement("latsig", tempgps.latsig);
            jobject.addElement("lonsig", tempgps.lonsig);
            jobject.addElement("heightsig", tempgps.heightsig);
            message = jobject.to_json_string();
            log_write(agent->nodeName, agent->agentName, currentmjd(), "", "mean", message);
            //                string result;
            //                data_execute("rsync -auv " + ggapath + " ", result);
            //                data_execute("rsync -auv " + gstpath, result);
            log_move(agent->nodeName, agent->agentName);
            gstmjd = currentmjd();
            ggamjd = currentmjd();
            et.reset();
        }
    }
}
