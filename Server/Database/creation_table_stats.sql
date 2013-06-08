CREATE TABLE IF NOT EXISTS `stats` (
  `id` int(11) NOT NULL,
  `getversion` int(11) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

insert into `stats` (`id`,`getversion`) values (0,0);