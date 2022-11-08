/*
 * Fledge IEC 61850 south plugin.
 *
 * Copyright (c) 2020, RTE (https://www.rte-france.com)
 *
 * Released under the Apache 2.0 Licence
 *
 * Author: Estelle Chigot, Colin Constans, Akli Rahmoun
 */

// C++ standard library headers
#include <string>

// Fledge headers
#include <plugin_api.h>
#include <logger.h>
#include <reading.h>

// local library
#include "./version.h"
#include "./plugin.h"
#include "./iec61850.h"

using INGEST_CB = void (*)(void *, Reading);

/**
 * The 61850 plugin interface
 */
extern "C" {
    static const PLUGIN_INFORMATION info = {
        PLUGIN_NAME,              // Name
        VERSION,                  // Version
        SP_ASYNC,                 // Flags
        PLUGIN_TYPE_SOUTH,        // Type
        "1.0.0",                  // Interface version
        default_config            // Default configuration
    };

    /**
     * Return the information about this plugin
     */
    PLUGIN_INFORMATION *plugin_info()
    {
        Logger::getLogger()->info("61850 Config is %s", info.config);
        return const_cast<PLUGIN_INFORMATION*>(&info);  // NOSONAR (Fledge API)
    }

    PLUGIN_HANDLE plugin_init(ConfigCategory *config)  // NOSONAR (Fledge API)
    {
        Logger::getLogger()->setMinLevel("info");

        IEC61850 *iec61850;
        Logger::getLogger()->info("Initializing the plugin");
        iec61850 = new IEC61850();

        if (config) {
            iec61850->setConfig(*config);
        }

        return (PLUGIN_HANDLE) iec61850;
    }

    /**
     * Start the Async handling for the plugin
     */
    void plugin_start(PLUGIN_HANDLE handle)
    {
        if (!handle) {
            return;
        }

        Logger::getLogger()->info("Starting the plugin");
        auto iec61850 = static_cast<IEC61850 *>(handle);
        Logger::getLogger()->setMinLevel(iec61850->getLogMinLevel());
        iec61850->start();
    }

    /**
     * Register ingest callback
     */
    void plugin_register_ingest(PLUGIN_HANDLE handle, INGEST_CB ingestCallback, void *data)  // NOSONAR (Fledge API)
    {
        if (!handle) {
            throw std::exception();
        }

        auto iec61850 = static_cast<IEC61850 *>(handle);
        iec61850->registerIngest(data, ingestCallback);
    }

    /**
     * Poll for a plugin reading
     */
    Reading plugin_poll(PLUGIN_HANDLE)  // NOSONAR (Fledge API)
    {
        throw std::runtime_error(
            "IEC_61850 is an async plugin, poll should not be called");
    }

    /**
     * Reconfigure the plugin
     *
     */
    void plugin_reconfigure(PLUGIN_HANDLE handle, std::string &newConfig)  // NOSONAR (Fledge API)
    {
        if (!handle) {
            Logger::getLogger()->warn("plugin_reconfigure: PLUGIN_HANDLE is null");
            return;
        }

        ConfigCategory config("new", newConfig);
        auto iec61850 = static_cast<IEC61850 *>(handle);
        iec61850->stop();
        iec61850->setConfig(config);
        Logger::getLogger()->setMinLevel(iec61850->getLogMinLevel());
        iec61850->start();
    }

    /**
     * Shutdown the plugin
     */
    void plugin_shutdown(PLUGIN_HANDLE handle)
    {
        if (!handle) {
            Logger::getLogger()->warn("plugin_shutdown: PLUGIN_HANDLE is null");
            return;
        }

        IEC61850 *iec61850 = nullptr;
        iec61850 = static_cast<IEC61850 *>(handle);

        if (nullptr != iec61850) {
            iec61850->stop();
            delete iec61850;
            iec61850 = nullptr;
        }
    }
}  // end of 'extern "C"'
