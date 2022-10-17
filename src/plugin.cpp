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
#include <config_category.h>
#include <logger.h>

// local library
#include "./version.h"
#include "./iec61850.h"

typedef void (*INGEST_CB)(void *, Reading);

#define PLUGIN_NAME "iec61850"

/**
 * Default configuration
 */

// *INDENT-OFF* (disable 'astyle' tool on this section)
static const char *const default_config = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "default" : PLUGIN_NAME,
        "readonly" : "true"
    },

    "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "default" : "iec61850",
        "displayName" : "Asset Name",
        "order" : "1",
        "mandatory" : "true"
    },

    "ip" : {
        "description" : "IP of the Server",
        "type" : "string",
        "default" : "127.0.0.1",
        "displayName" : "61850 Server IP",
        "order" : "2"
    },

    "port" : {
        "description" : "Port number of the 61850 server",
        "type" : "integer",
        "default" : "102",
        "displayName" : "61850 Server port"
    },

    "IED Model" : {
        "description" : "Name of the 61850 IED model",
        "type" : "string",  // IedModel
        "default" : "simpleIO",
        "displayName" : "61850 Server IedModel"
    },
    "Logical Device" : {
        "description" : "Logical device of the 61850 server",
        "type" : "string",  // LogicalDevice
        "default" : "GenericIO",
        "displayName" : "61850 Server logical device"
    },

    "Logical Node" : {
        "description" : "Logical node of the 61850 server",
        "type" : "string",  // LogicalNode
        "default" : "GGIO1",
        "displayName" : "61850 Server logical node"
    },

    "CDC" : {
        "description" : "CDC name of the 61850 server",
        "type" : "string",  // CDC_SAV
        "default" : "SPCSO1",
        "displayName" : "61850 Server CDC_SAV"
    },

    "Data Attribute" : {
        "description" : "Data attribute of the CDC",
        "type" : "string",  // dataAttribute
        "default" : "stVal",
        "displayName" : "61850 Server data attribute"
    },

    "Functional Constraint" : {
        "description" : "Functional constraint of the 61850 server",
        "type" : "string",  // FC
        "default" : "ST",
        "displayName" : "61850 Server functional constraint"
    }
});
// *INDENT-ON*

/**
 * The 61850 plugin interface
 */
extern "C" {
    static PLUGIN_INFORMATION info = {
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
        return &info;
    }

    PLUGIN_HANDLE plugin_init(ConfigCategory *config)
    {
        IEC61850 *iec61850;
        Logger::getLogger()->info("Initializing the plugin");
        std::string ip = "127.0.0.1";
        std::string model = "testmodel";
        std::string logicalNode;
        std::string logicalDevice;
        std::string cdc;
        std::string attribute;
        std::string fc;
        uint16_t port = 8102;

        if (config->itemExists("ip")) {
            ip = config->getValue("ip");
        }

        if (config->itemExists("port")) {
            port = static_cast<uint16_t>(stoi(config->getValue("port")));
        }

        if (config->itemExists("IED Model")) {
            model = (config->getValue("IED Model"));
        }

        if (config->itemExists("Logical Device")) {
            logicalDevice = config->getValue("Logical Device");
        }

        if (config->itemExists("Logical Node")) {
            logicalNode = config->getValue("Logical Node");
        }

        if (config->itemExists("CDC")) {
            cdc = config->getValue("CDC");
        }

        if (config->itemExists("Functional Constraint")) {
            fc = config->getValue("Functional Constraint");
        }

        if (config->itemExists("Data Attribute")) {
            attribute = config->getValue("Data Attribute");
        }

        iec61850 = new IEC61850(ip.c_str(),
                                port,
                                model,
                                logicalNode,
                                logicalDevice,
                                cdc,
                                attribute,
                                fc);

        if (config->itemExists("asset")) {
            iec61850->setAssetName(config->getValue("asset"));
        } else {
            iec61850->setAssetName("iec61850");
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
        IEC61850 *iec61850 = static_cast<IEC61850 *>(handle);
        iec61850->start();
    }

    /**
     * Register ingest callback
     */
    void plugin_register_ingest(PLUGIN_HANDLE handle, INGEST_CB cb, void *data)
    {
        if (!handle) {
            throw new std::exception();
        }

        IEC61850 *iec61850 = static_cast<IEC61850 *>(handle);
        iec61850->registerIngest(data, cb);
    }

    /**
     * Poll for a plugin reading
     */
    Reading plugin_poll(PLUGIN_HANDLE handle)
    {
        throw std::runtime_error(
            "IEC_61850 is an async plugin, poll should not be called");
    }

    /**
     * Reconfigure the plugin
     *
     */
    void plugin_reconfigure(PLUGIN_HANDLE handle, std::string &newConfig)
    {
        if (!handle) {
            Logger::getLogger()->warn("plugin_reconfigure: PLUGIN_HANDLE is null");
            return;
        }

        ConfigCategory config("new", newConfig);
        IEC61850 *iec61850 = static_cast<IEC61850 *>(handle);
        std::unique_lock<std::mutex> guard2(iec61850->loopLock);
        iec61850->loopActivated = false;
        iec61850->loopThread.join();
        iec61850->stop();

        if (config.itemExists("ip")) {
            std::string ip_address = config.getValue("ip");
            iec61850->setIp(ip_address.c_str());
        }

        if (config.itemExists("port")) {
            uint16_t port = static_cast<uint16_t>(stoi(config.getValue("port")));
            iec61850->setPort(port);
        }

        if (config.itemExists("IED Model")) {
            std::string model = (config.getValue("IED Model"));
            iec61850->setModel(model);
        }

        if (config.itemExists("Logical Device")) {
            std::string logicalDevice = config.getValue("Logical Device");
            iec61850->setLogicalDevice(logicalDevice);
        }

        if (config.itemExists("Logical Node")) {
            std::string logicalNode = config.getValue("Logical Node");
            iec61850->setLogicalNode(logicalNode);
        }

        if (config.itemExists("CDC")) {
            std::string cdc_name = config.getValue("CDC");
            iec61850->setCdc(cdc_name);
        }

        if (config.itemExists("Data Attribute")) {
            std::string data_attribute = config.getValue("Data Attribute");
            iec61850->setAttribute(data_attribute);
        }

        if (config.itemExists("Functional Constraint")) {
            std::string fc_name = config.getValue("Functional Constraint");
            iec61850->setFc(fc_name);
        }

        if (config.itemExists("asset")) {
            iec61850->setAssetName(config.getValue("asset"));
        } else {
            iec61850->setAssetName("iec61850");
        }

        iec61850->start();
        guard2.unlock();
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
