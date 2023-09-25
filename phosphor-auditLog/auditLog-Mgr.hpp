#pragma once

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Logging/AuditLog/server.hpp>

#include <string>

namespace phosphor
{
namespace auditLog_Mgr
{

using ALIface = sdbusplus::xyz::openbmc_project::Logging::server::AuditLog;
using ALObject = sdbusplus::server::object_t<ALIface>;

/** @class ALManager
 *  @brief Configuration for AuditLog server
 *  @details A concrete implementation of the
 *  xyz.openbmc_project.Logging.AuditLog API, in order to
 *  provide audit log support.
 */
class ALManager : public ALObject
{
  public:
    ALManager() = delete;
    ALManager(const ALManager&) = delete;
    ALManager& operator=(const ALManager&) = delete;
    ALManager(ALManager&&) = delete;
    ALManager& operator=(ALManager&&) = delete;
    virtual ~ALManager() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     */
    ALManager(sdbusplus::bus_t& bus, const std::string& path) :
        ALObject(bus, path.c_str())
    {
        lg2::debug("Constructing ALManager Path={PATH}", "PATH", path);
    }

    virtual void parseAuditLog(std::string filePath) override
    {
        lg2::debug("Method parseAuditLog filePath={FILEPATH}", "FILEPATH",
                   filePath);
    }

    virtual sdbusplus::message::unix_fd getAuditLog() override
    {
        int fd = -1;

        lg2::debug("Method getAuditLog");
        if (fd == -1)
        {
            throw sdbusplus::xyz::openbmc_project::Common::File::Error::Open();
        }

        return fd;
    }

    virtual void logEvent(std::string operation, std::string username,
                          std::string ipAddress, std::string hostname,
                          Result result, std::string detailData) override
    {
        lg2::debug(
            "Method logEvent op={OPERATION} user={USER} addr={ADDR} host={HOST} result={RESULT} detail={DETAIL}",
            "OPERATION", operation, "USER", username, "ADDR", ipAddress, "HOST",
            hostname, "RESULT", result, "DETAIL", detailData);
    }

  private:
};

} // namespace auditLog_Mgr
} // namespace phosphor
