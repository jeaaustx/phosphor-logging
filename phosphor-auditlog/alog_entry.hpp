#pragma once

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Logging/AuditLog/AuditEntry/server.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace phosphor::auditlog
{

using ALEntryIface =
    sdbusplus::server::xyz::openbmc_project::logging::audit_log::AuditEntry;
using PropertiesVariant = ALEntryIface::PropertiesVariant;

/** @class ALEntry
 *  @brief Implementation of AuditEntry on DBus
 *  @details A concrete implementation of the
 *  xyz.openbmc_project.Logging.AuditLog.AuditEntry API, in order to
 *  provide audit log support.
 */
class ALEntry : public ALEntryIface
{
  public:
    ALEntry() = delete;
    ALEntry(const ALEntry&) = delete;
    ALEntry& operator=(const ALEntry&) = delete;
    ALEntry(ALEntry&&) = delete;
    ALEntry& operator=(ALEntry&&) = delete;
    virtual ~ALEntry() = default;

    /** @brief Constructor to put object onto bus at a dbus path.
     *  @param[in] bus - Bus to attach to.
     *  @param[in] path - Path to attach at.
     *  @param[in] vals - Property values to be set when this interface is
     *                    added to the bus.
     */
    ALEntry(sdbusplus::bus_t& bus, const std::string& path,
            const std::map<std::string, PropertiesVariant>& vals,
            const std::string_view id) :
        ALEntryIface(bus, path.c_str(), vals, true),
        entryID(path)
    {
        lg2::debug("ALEntry: {ENTRYID} {PATH} {BUS}", "ENTRYID", id, "PATH",
                   path, "BUS", bus.get_unique_name());
    };

#if 0
    /* Couldn't get removal from D-Bus to work for the objects. */
    void erase(const std::string& path)
    {
            lg2::debug("ALEntry::erase {PATH}", "PATH", path);
            ALEntryIface::erase(path);
    };
    /* What if I just define it? */
    void erase(const std::string& path);
#endif

  private:
    std::string entryID;
};

/** @class ALEntryList
 *  @brief Manages ALEntry members
 *  @details Creates ALEntry for particular ALManager
 */
class ALEntryList
{
  public:
    ALEntryList(const ALEntryList&) = delete;
    ALEntryList& operator=(const ALEntryList&) = delete;
    ALEntryList(ALEntryList&&) = delete;
    ALEntryList& operator=(ALEntryList&&) = delete;
    ~ALEntryList() = default;

    ALEntryList() : maxEntries(0)
    {
        lg2::debug("ALEntryList constructed 0");
    }

    /** @brief Constructor to initialize collection of audit entries
     *  @param[in] maxEvents Maximum number of events to retain.
     */
    ALEntryList(uint32_t maxEvents) : maxEntries(maxEvents)
    {
        /* TODO: Initialize auditEntries */
        lg2::debug("ALEntryList constructed {MAXEVENTS}", "MAXEVENTS",
                   maxEvents);
    };

    void clearEntries()
    {
        if (!auditEntries.empty())
        {
            lg2::debug("clearEntries: Removing {SIZE} audit entries", "SIZE",
                       auditEntries.size());
#if 1
            auditEntries.clear();
#else
            /* Need to clear entries one at a time to make sure interface is
             * removed? Nope, this didn't work either.
             */
            for (auto entry = auditEntries.begin(); entry != auditEntries.end();
                 entry++)
            {
                auto path = entry->first;
                lg2::debug("Removing entry {PATH}", "PATH", path);
                auditEntries.erase(path);
            }

            lg2::debug("clearEntries: After erase {SIZE} audit entries", "SIZE",
                       auditEntries.size());
#endif
        }
    }

    void setMax(uint32_t maxEvents)
    {
        lg2::debug("setMax {MAXEVENTS}", "MAXEVENTS", maxEvents);
#if 0
        /* Removing entries is not working, so just skip for now.
         * Entries are removed from in-memory auditEntries list but not from
         * the D-Bus. Seems like the base class erase() should be doing it?
         */
        clearEntries();
#endif
        maxEntries = maxEvents;
    }

    void createEntry(std::string_view pathID, nlohmann::json entry,
                     sdbusplus::bus_t& bus)
    {
        lg2::debug("createEntry: {ENTRY} {BUS}", "ENTRY", entry, "BUS",
                   bus.get_unique_name());

        std::string path =
            std::format("/xyz/openbmc_project/logging/auditlog/{}", pathID);

        /* Convert json into property pairs.
         * Note: This is for prototype only. If doing this normally then the
         * parsing would write to this format directly.
         */
        std::map<std::string, PropertiesVariant> entryData;
        for (const auto& item : entry.items())
        {
            lg2::debug("Next item {KEY}:{VALUE}", "KEY", item.key(), "VALUE",
                       item.value());
            if (item.key() == "EventTimestamp")
            {
                uint64_t ts = item.value();
                lg2::debug("Adding item {KEY}:{VALUE}", "KEY", item.key(),
                           "VALUE", item.value());
                entryData.emplace(item.key(), ts);
            }
            else if (item.key() == "MessageArgs")
            {
                if (!item.value().is_array())
                {
                    lg2::debug("Skipping item {KEY}:{VALUE} expected array",
                               "KEY", item.key(), "VALUE", item.value());
                }
                else
                {
                    lg2::debug("Adding item {KEY}:{VALUE}", "KEY", item.key(),
                               "VALUE", item.value());
                    std::vector<std::string> itemData;
                    for (const auto& entry : item.value().items())
                    {
                        itemData.emplace_back(entry.value());
                    }
                    entryData.emplace(item.key(), itemData);
                }
            }
            else
            {
                /* Everything else is just a plain string, just handle */
                lg2::debug("Adding item {KEY}:{VALUE}", "KEY", item.key(),
                           "VALUE", item.value());
                entryData.emplace(item.key(), std::string(item.value()));
            }
        }

        lg2::debug("ALEntry: {PATHID} {PATH}", "PATHID", pathID, "PATH", path);
        auto auditEntry = std::make_unique<ALEntry>(bus, path, entryData,
                                                    pathID);
        auditEntries.emplace(std::move(path), std::move(auditEntry));
    }

  private:
    /**
     * @brief The map used to keep track of Audit Log entry pointers.
     */
    std::map<std::string,
             std::unique_ptr<sdbusplus::common::xyz::openbmc_project::logging::
                                 audit_log::AuditEntry>>
        auditEntries;

    size_t maxEntries;
};

} // namespace phosphor::auditlog
