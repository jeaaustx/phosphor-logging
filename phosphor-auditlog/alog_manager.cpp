#include "alog_manager.hpp"

#include "alog_entry.hpp"
#include "alog_parser.hpp"
#include "alog_utils.hpp"

#include <fcntl.h>
#include <libaudit.h>

#include <phosphor-logging/lg2.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/source/event.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>

#include <cstring>
#include <memory>
#include <string>

namespace phosphor::auditlog
{

sdbusplus::message::unix_fd ALManager::getAuditLog()
{
#ifdef AUDITLOG_KEEP_JSONFILE
    ALParseFile parsedFile("/tmp/auditLog.json");
#else
    ALParseFile parsedFile;
#endif

    // Initialize to parse full audit log
    ALParseAll auditParser(parsedFile);

    lg2::debug("Method GetAuditLog: {FILEPATH}", "FILEPATH",
               parsedFile.getPath());

    // Parse all the events
    auditParser.doParse();

    /* Get file descriptor to return.
     * openParseFD() throws an error if it fails to open the file.
     */
    auto fd = openParseFD(parsedFile);

    /* Schedule the fd to be closed by sdbusplus when it sends it back over
     * D-Bus.
     */
    sdeventplus::Event event = sdeventplus::Event::get_default();
    fdCloseEventSource = std::make_unique<sdeventplus::source::Defer>(
        event, std::bind(&ALManager::closeFD, this, fd, std::placeholders::_1));

    return fd;
}

sdbusplus::message::unix_fd ALManager::getLatestEntries(uint32_t maxEvents)
{
#ifdef AUDITLOG_KEEP_JSONFILE
    ALParseFile parsedFile("/tmp/auditEntries.json");
#else
    ALParseFile parsedFile;
#endif

    lg2::debug("Method GetLatestEntries: {FILE} maxEvents: {MAXCOUNT}", "FILE",
               parsedFile.getPath(), "MAXCOUNT", maxEvents);

    ALParseLatest auditParser(maxEvents, parsedFile);

    // Parse events up to maxEvents specified
    auditParser.doParse();

    /* Get file descriptor to return.
     * openParseFD() throws an error if it fails to open the file.
     */
    auto fd = openParseFD(parsedFile);

    /* Schedule the fd to be closed by sdbusplus when it sends it back over
     * D-Bus.
     */
    sdeventplus::Event event = sdeventplus::Event::get_default();
    fdCloseEventSource = std::make_unique<sdeventplus::source::Defer>(
        event, std::bind(&ALManager::closeFD, this, fd, std::placeholders::_1));

    return fd;
}

void ALManager::populateAuditLog(uint32_t maxEvents)
{
    ALParseFile parsedFile("/tmp/auditDBusEntries.json");
    size_t maxLeftCount = maxEvents;
    size_t parsedCount = 0;

    lg2::debug("Method PopulateAuditLog: {FILE} maxEvents: {MAXCOUNT}", "FILE",
               parsedFile.getPath(), "MAXCOUNT", maxEvents);

    ALParsePopulate auditParser(maxEvents, parsedFile, getBus());

    /* Clears any pre-existing entries and sets limit for new ones */
    auditEntries.setMax(maxEvents);

    while (parsedCount < maxLeftCount)
    {
        lg2::debug(
            "populateAuditLog: parsedCount: {PARSEDCOUNT} maxLeftCount: {MAXLEFT}",
            "PARSEDCOUNT", parsedCount, "MAXLEFT", maxLeftCount);
        maxLeftCount -= parsedCount;

        // Parse events up to maxEvents specified
        if (!auditParser.doParse(maxLeftCount))
        {
            // No more to process
            break;
        }

        /* TODO: For correct D-Bus object ordering, need to append subsequent
         * elements to existing list. Then do a single write of the list. (Need
         * to create objects oldest to newest.
         */
        // Add newest entries to auditEntries
        parsedCount = auditParser.writeParsedEntries(std::ref(auditEntries));
    }
}

int ALManager::openParseFD(const ALParseFile& parsedFile)
{
    // Confirm parsed file exists
    int fd = -1;
    std::error_code ec;

    fd = open(parsedFile.getPath().c_str(), O_RDONLY | O_NONBLOCK);
    if (fd == -1)
    {
        auto e = errno;
        lg2::error("Failed to open {PATH}: {ERRNO}", "ERRNO", e, "PATH",
                   parsedFile.getPath());
        throw sdbusplus::xyz::openbmc_project::Common::File::Error::Open();
    }

    lg2::debug("Opening parsedFile: {PARSEFD}", "PARSEFD", fd);

    return fd;
}

void ALManager::closeFD(int fd, sdeventplus::source::EventBase& /*source*/)
{
    lg2::debug("Closing parsedFile: {FD}", "FD", fd);
    close(fd);
    fdCloseEventSource.reset();
}

} // namespace phosphor::auditlog
