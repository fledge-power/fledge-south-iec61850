#ifndef CONFIGURATION_EXAMPLES_H_
#define CONFIGURATION_EXAMPLES_H_

#include <string>
#include <plugin_api.h>

const std::string configForReconfiguration = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "log min level" : {
        "description" : "minimum level for the Fledge logger (debug, info)",
        "type" : "string",
        "value" : "info",
        "displayName" : "logger minimum level",
        "order" : "1",
        "mandatory" : "true"
    },

    "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "value" : "iec61850",
        "displayName" : "Asset Name",
        "order" : "2",
        "mandatory" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                        },
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 8102
                        }
                    ]
                },
                "application_layer" : {
                }
            }
        })
    },
    "exchanged_data" : {
        "description" : "exchanged data list",
        "type" : "JSON",
        "displayName" : "Exchanged data list",
        "order" : "4",
        "value" : QUOTE({
            "exchanged_data": {
                "name" : "iec104client",
                "version" : "1.0",
                "Logical Device": "GenericIO",
                "Logical Node": "GGIO1",
                "CDC" : "AnIn1",
                "Data Attribute": "mag.f",
                "Functional Constraint": "MX"
            }
        })
    }
});

const std::string configWithoutLogLevel = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "asset" : {
        "description" : "Asset name",
        "type" : "string",
        "value" : "iec61850",
        "displayName" : "Asset Name",
        "order" : "2",
        "mandatory" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                        },
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 8102
                        }
                    ]
                },
                "application_layer" : {
                }
            }
        })
    },
    "exchanged_data" : {
        "description" : "exchanged data list",
        "type" : "JSON",
        "displayName" : "Exchanged data list",
        "order" : "4",
        "value" : QUOTE({
            "exchanged_data": {
                "name" : "iec104client",
                "version" : "1.0",
                "Logical Device": "GenericIO",
                "Logical Node": "GGIO1",
                "CDC" : "AnIn1",
                "Data Attribute": "mag.f",
                "Functional Constraint": "MX"
            }
        })
    }
});

const std::string configMissingProtocolStack = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    }
});

const std::string configProtocolStackWithParsingError = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3"
    }
});

const std::string configProtocolStackEmptyConf = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
        })
    }
});

const std::string configProtocolStackMissingTransport = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0"
            }
        })
    }
});

const std::string configProtocolStackMissingApplication = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackMissingIedName = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "connections" : [
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackIedNameBadFormat = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : 8102,
                    "connections" : [
                        {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackMissingConnection = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO"
                }
            }
        })
    }
});

const std::string configProtocolStackConnectionBadFormat = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : {
                            "srv_ip": "0.0.0.0",
                            "port" : 102
                    }
                }
            }
        })
    }
});

const std::string configProtocolStackMissingSrvIp = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "port" : 102
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackSrvIpBadFormat = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip" : 102
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackMissingPort = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip" : "0.0.0.0"
                        }
                    ]
                }
            }
        })
    }
});

const std::string configProtocolStackPortBadFormat = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip" : "0.0.0.0",
                            "port" : "8102"
                        }
                    ]
                }
            }
        })
    }
});


const std::string configProtocolStackInvalidIPAddress = QUOTE({
    "plugin" : {
        "description" : "iec61850 south plugin",
        "type" : "string",
        "value" : "iec61850",
        "readonly" : "true"
    },

    "protocol_stack" : {
        "description" : "protocol stack parameters",
        "type" : "JSON",
        "displayName" : "Protocol stack parameters",
        "order" : "3",
        "value" : QUOTE({
            "protocol_stack" : {
                "name" : "iec61850client",
                "version" : "1.0",
                "transport_layer" : {
                    "ied_name" : "simpleIO",
                    "connections" : [
                        {
                            "srv_ip" : "192.168.1.345",
                            "port" : 8102
                        }
                    ]
                }
            }
        })
    }
});

#endif
