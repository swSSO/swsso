CREATE TABLE IF NOT EXISTS `config` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `typeapp` varchar(3) COLLATE utf8_unicode_ci NOT NULL,
  `title` varbinary(128) NOT NULL,
  `url` varbinary(144) DEFAULT NULL,
  `id1Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id1Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id2Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id2Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id3Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id3Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id4Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id4Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id5Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id5Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `pwdName` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `validateName` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `bKBSim` tinyint(1) NOT NULL DEFAULT '0',
  `szKBSim` varchar(200) COLLATE utf8_unicode_ci DEFAULT NULL,
  `szName` varbinary(64) DEFAULT NULL,
  `szFullPathName` varbinary(272) DEFAULT NULL,
  `categId` int(11) NOT NULL DEFAULT '0',
  `lastModified` datetime DEFAULT NULL,
  `domainId` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;
