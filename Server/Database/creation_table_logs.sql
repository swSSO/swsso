CREATE TABLE IF NOT EXISTS `logs` (
  `horodate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `title` varchar(200) COLLATE utf8_unicode_ci NOT NULL,
  `url` varchar(400) COLLATE utf8_unicode_ci NOT NULL,
  `result` int(11) NOT NULL,
  `domainId` int(11) NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci ;
