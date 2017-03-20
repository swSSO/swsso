CREATE TABLE IF NOT EXISTS `admins` (
  `userid` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `userpwd` varchar(255) NOT NULL,
  `userrole` varchar(50) NOT NULL,
  `userfirstname` varchar(50) NOT NULL,
  `userlastname` varchar(50) NOT NULL,
  `userlocked` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`userid`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
