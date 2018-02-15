#if 1
//#include "libevent.h"
#include "examples.h"

natsConnection  *conn  = NULL;
natsSubscription    *sub   = NULL;
natsStatistics  *stats = NULL;
int64_t         last   = 0;
natsStatus      s;
natsMsg             *msg   = NULL;

//struct event_base   *evLoop= NULL;

#if 0
static void
onMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure)
{
    if (print)
        printf("Received msg: %s - %.*s\n",
               natsMsg_GetSubject(msg),
               natsMsg_GetDataLength(msg),
               natsMsg_GetData(msg));

    if (start == 0)
        start = nats_Now();

    // We should be using a mutex to protect those variables since
    // they are used from the subscription's delivery and the main
    // threads. For demo purposes, this is fine.
    if (++count == total)
        elapsed = nats_Now() - start;

    natsMsg_Destroy(msg);
}
#endif

void dhcps_nats_connect(int argc, char **argv)
{
    natsOptions     *opts  = NULL;
    
    static const char *usage = ""\
    "-txt           text to send (default is 'hello')\n" \
    "-count         number of messages to send\n";

    opts = parseArgs(argc, argv, usage);

    printf("Sending %" PRId64 " messages to subject '%s'\n", total, subj);

    s = natsConnection_Connect(&conn, opts);

    if (s == NATS_OK)
        s = natsStatistics_Create(&stats);

    if (s == NATS_OK)
        s = natsConnection_SubscribeSync(&sub, conn, subj);//, onMsg, NULL);

/*    if (s == NATS_OK)
        start = nats_Now();
*/
/*    for (count = 0; (s == NATS_OK) && (count < total); count++)
    {
        s = natsConnection_PublishString(conn, subj, txt);

        if (nats_Now() - last >= 1000)
        {
            s = printStats(STATS_OUT, conn, NULL, stats);
            last = nats_Now();
        }
    }
*/
    if (s == NATS_OK)
        s = natsConnection_FlushTimeout(conn, 1000);

    if (s == NATS_OK)
    {
        printPerf("Sent", total, start, elapsed);
    }
    else
    {
        printf("Error: %d - %s\n", s, natsStatus_GetText(s));
        nats_PrintLastErrorStack(stderr);
    }

    // Destroy all our objects to avoid report of memory leak
//    natsStatistics_Destroy(stats);
//    natsConnection_Destroy(conn);
//    natsOptions_Destroy(opts);

    // To silence reports of memory still in used with valgrind
//    nats_Close();

    return;
}

#endif


void dhcps_nats_publish(void)
{
//    if (s == NATS_OK)
//        s = natsStatistics_Create(&stats);

    if (s == NATS_OK)
        start = nats_Now();

    for (count = 0; (s == NATS_OK) && (count < total); count++)
    {
        s = natsConnection_PublishString(conn, "AUTH", "umesh:redback");

        if (nats_Now() - last >= 1000)
        {
            s = printStats(STATS_OUT, conn, NULL, stats);
            last = nats_Now();
        }
    }

    if (s == NATS_OK)
        s = natsSubscription_NextMsg(&msg, sub, 10000);

    if(msg != NULL) {
        printf("RESP : subject:%s, reply:%s, data:%s\n",
            (const char*)natsMsg_GetSubject(msg),
            (const char*)natsMsg_GetReply(msg),
            (const char*)natsMsg_GetData(msg));
    } else {
        printf("MSG is NULL!!!");
    }

    if (start == 0)
        start = nats_Now();

    if (nats_Now() - last >= 1000)
    {
        s = printStats(STATS_IN|STATS_COUNT, conn, sub, stats);
        last = nats_Now();
    }

    natsMsg_Destroy(msg);        

//    if (s == NATS_OK)
//        s = natsConnection_FlushTimeout(conn, 1000);

    if (s == NATS_OK)
    {
        printPerf("Sent", total, start, elapsed);
    }
    else
    {
        printf("Error: %d - %s\n", s, natsStatus_GetText(s));
        nats_PrintLastErrorStack(stderr);
    }

    return;
}


#if 0

static void
onMsg(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure)
{
    if (print)
        printf("Received msg: %s - %.*s\n",
               natsMsg_GetSubject(msg),
               natsMsg_GetDataLength(msg),
               natsMsg_GetData(msg));

    natsMsg_Destroy(msg);

    if (start == 0)
        start = nats_Now();

    // We should be using a mutex to protect those variables since
    // they are used from the subscription's delivery and the main
    // threads. For demo purposes, this is fine.
    if (++count == total)
    {
        elapsed = nats_Now() - start;

        natsConnection_Close(nc);
    }
}

int dhcp_nats_receive(int argc, char **argv)
{
    natsOptions     *opts  = NULL;
    
    static const char *usage = ""\
    "-gd            use global message delivery thread pool\n" \
    "-count         number of expected messages\n";

    nats_Open(-1);    
    opts = parseArgs(argc, argv, usage);

    printf("Listening on '%s'.\n", subj);

    // One time initialization of things that we need.
    natsLibevent_Init();

    // Create a loop.
    evLoop = event_base_new();
    if (evLoop == NULL)
        s = NATS_ERR;

    // Indicate which loop and callbacks to use once connected.
    if (s == NATS_OK)
        s = natsOptions_SetEventLoop(opts, (void*) evLoop,
                                     natsLibevent_Attach,
                                     natsLibevent_Read,
                                     natsLibevent_Write,
                                     natsLibevent_Detach);

    if (s == NATS_OK)
        s = natsConnection_Connect(&conn, opts);

    if (s == NATS_OK)
        s = natsConnection_Subscribe(&sub, conn, subj, onMsg, NULL);

    // For maximum performance, set no limit on the number of pending messages.
    if (s == NATS_OK)
        s = natsSubscription_SetPendingLimits(sub, -1, -1);

    // Run the event loop.
    // This call will return when the connection is closed (either after
    // receiving all messages, or disconnected and unable to reconnect).
    if (s == NATS_OK)
        event_base_dispatch(evLoop);

    if (s == NATS_OK)
    {
        printPerf("Received", count, start, elapsed);
    }
    else
    {
        printf("Error: %d - %s\n", s, natsStatus_GetText(s));
        nats_PrintLastErrorStack(stderr);
    }

    // Destroy all our objects to avoid report of memory leak
    natsSubscription_Destroy(sub);
    natsConnection_Destroy(conn);
    natsOptions_Destroy(opts);

    if (evLoop != NULL)
        event_base_free(evLoop);

    // To silence reports of memory still in used with valgrind
    nats_Close();
//    libevent_global_shutdown();

    return 0;
}

#endif

