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

CREATE TABLE IF NOT EXISTS `logs` (
  `horodate` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `title` varchar(200) COLLATE utf8_unicode_ci NOT NULL,
  `url` varchar(400) COLLATE utf8_unicode_ci NOT NULL,
  `result` int(11) NOT NULL,
  `domainId` int(11) NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci ;
