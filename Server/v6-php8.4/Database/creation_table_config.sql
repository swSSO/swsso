-- -----------------------------------------------------------------------------
--                                   swSSO
--                 Copyright (C) 2004-2026 - Sylvain WERDEFROY
--                          https://github.com/swSSO
-- -----------------------------------------------------------------------------
--  
--   This file is part of swSSO.
--   
--   swSSO is free software: you can redistribute it and/or modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation, either version 3 of the License, or
--   (at your option) any later version.
-- 
--   swSSO is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.
-- 
--   You should have received a copy of the GNU General Public License
--   along with swSSO.  If not, see <http://www.gnu.org/licenses/>.
--  
-- ------------------------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `config` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `active` tinyint(1) NOT NULL DEFAULT '1',
  `typeapp` varchar(3) COLLATE utf8_unicode_ci NOT NULL,
  `title` varchar(90) COLLATE utf8_unicode_ci NOT NULL,
  `url` varchar(256) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id1Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id1Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id2Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id2Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id3Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id3Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id4Name` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `id4Type` varchar(5) COLLATE utf8_unicode_ci DEFAULT NULL,
  `pwdName` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `validateName` varchar(90) COLLATE utf8_unicode_ci DEFAULT NULL,
  `bKBSim` tinyint(1) NOT NULL DEFAULT '0',
  `szKBSim` varchar(400) COLLATE utf8_unicode_ci DEFAULT NULL,
  `szName` varchar(50) COLLATE utf8_unicode_ci DEFAULT NULL,
  `szFullPathName` varchar(255) COLLATE utf8_unicode_ci DEFAULT NULL,
  `categId` int(11) NOT NULL DEFAULT '0',
  `lastModified` datetime DEFAULT NULL,
  `pwdGroup` tinyint (1) DEFAULT '-1',
  `autoLock` tinyint (1) DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;
