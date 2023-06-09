DROP TABLE IF EXISTS `cfg_channel`;
CREATE TABLE `cfg_channel` (
  `channelId` int(16) NOT NULL,
  `protocolFile` varchar(256) DEFAULT NULL,
  `protocolName` varchar(256) DEFAULT NULL,
  `protocolType` int(11) DEFAULT NULL,
  `channelInterval` int(11) DEFAULT NULL,
  `channelName` varchar(256) DEFAULT NULL,
  `autoOpen` int(11) DEFAULT NULL,
  `transChannelId` int(11) DEFAULT NULL,
  PRIMARY KEY (`channelId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `cfg_device`;
CREATE TABLE `cfg_device` (
  `deviceId` int(11) NOT NULL,
  `channelId` int(11) NOT NULL,
  `devicetypeId` int(11) DEFAULT NULL,
  `deviceName` varchar(64) DEFAULT NULL,
  `protocolFile` varchar(256) DEFAULT NULL,
  `protocolName` varchar(256) DEFAULT NULL,
  `address` int(11) DEFAULT NULL,
  `break` int(11) DEFAULT NULL,
  `isRun` int(11) DEFAULT NULL,
  `reSend` int(11) DEFAULT NULL,
  `transDeviceId` int(11) DEFAULT NULL,
  PRIMARY KEY (`deviceId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS `cfg_port`;
CREATE TABLE `cfg_port` (
  `portId` int(11) NOT NULL,
  `channelId` int(11) NOT NULL,
  `portType` int(11) NOT NULL,
  `portName` varchar(32) DEFAULT NULL,
  `portNum` int(11) DEFAULT NULL,
  `baudRate` int(11) DEFAULT NULL,
  `parity` int(11) DEFAULT NULL,
  `dataBits` int(11) DEFAULT NULL,
  `stopBits` int(11) DEFAULT NULL,
  `localAddress` varchar(32) DEFAULT NULL,
  `localPort` int(11) DEFAULT NULL,
  `remoteAddress` varchar(32) DEFAULT NULL,
  `remotePort` int(11) DEFAULT NULL,
  `isMulticast` int(11) DEFAULT NULL,
  `isBackup` int(11) DEFAULT NULL,
  PRIMARY KEY (`portId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
