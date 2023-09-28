#include "auditLogParser.hpp"

#include "auditLogMgr.hpp"

#include <auparse.h>
#include <libaudit.h>

#include <phosphor-logging/lg2.hpp>

#include <cstring>
#include <string>

namespace phosphor
{
namespace auditlog
{

/**
 * @brief Moves parser to point to next event
 *
 * @return false when no more events exist, or on error
 */
bool ALParser::auditNextEvent()
{
    bool haveEvent = false;
    int rc;

    rc = auparse_next_event(au);
    switch (rc)
    {
        case 1:
            /* Success, pointing to next event */
            haveEvent = true;
            break;
        case 0:
            /* No more events */
            haveEvent = false;
            break;
        case -1:
        default:
            /* Failure */
            lg2::error("Failed to parse next event");
            haveEvent = false;
            break;
    }

    return haveEvent;
}

void ALParser::parseEvent()
{
    unsigned int nRecords = auparse_get_num_records(au);

    int eventType = auparse_get_type(au);
    if (eventType == AUDIT_USYS_CONFIG)
    {
        lg2::debug("Found one of our events");

        unsigned long serial = auparse_get_serial(au);
        time_t eventTime = auparse_get_time(au);
#if 0
                /* This gives milliseconds, seconds, serial in one, can use to
                 * build unique event ID as well as timestamp
                 */
                 typedef struct
{
        time_t sec;             // Event seconds
        unsigned int milli;     // millisecond of the timestamp
        unsigned long serial;   // Serial number of the event
        const char *host;       // Machine's node name
} au_event_t;
                 /* returns NULL on error */
                 const au_event_t *auparse_get_timestamp(auparse_state_t *au);
#endif

        /* Walk the fields, this is per record */
        unsigned int nFields = auparse_get_num_fields(au);

        lg2::debug("serial={SERIAL} time={ETIME} nFields={NFIELDS}", "SERIAL",
                   serial, "ETIME", eventTime, "NFIELDS", nFields);

        int fieldIdx = 0;
        int frc;
        do
        {
            fieldIdx++;
            /* can return nullptr */
            const char* fieldName = auparse_get_field_name(au);
            const char* fieldTxt = auparse_get_field_str(au);

            lg2::debug("Field {NFIELD} : {FIELDNAME} = {FIELDSTR}", "NFIELD",
                       fieldIdx, "FIELDNAME", fieldName, "FIELDSTR", fieldTxt);
        } while ((frc = auparse_next_field(au)) == 1);
    }

    lg2::debug("nRecords={NRECS} type={EVENTTYPE}", "NRECS", nRecords,
               "EVENTTYPE", eventType);

    /* The event itself is a record, and may be the only
     * one
     */
    auto eventText = auparse_get_record_text(au);

    lg2::debug("Event Text={TEXT}", "TEXT", eventText);

    /* Handle any additional records for this event */
    for (unsigned int iter = 1; iter < nRecords; iter++)
    {
        auto rc = auparse_next_record(au);
        lg2::debug("Record={ITER} rc={RC}", "ITER", iter, "RC", rc);

        switch (rc)
        {
            case 1:
            {
                /* Success finding record, dump its text */
                auto recText = auparse_get_record_text(au);

                lg2::debug("Record Text={TEXT}", "TEXT", recText);
            }
            break;
            case 0:
                /* No more records, something is confused! */
                lg2::error("Record count and records out of sync");
                break;
            case -1:
            default:
                /* Error */
                lg2::error("Failed on record number={ITER}", "ITER", iter);
                break;
        }
    }
}

} // namespace auditlog
} // namespace phosphor
