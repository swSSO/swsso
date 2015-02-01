-- -----------------------------------------------------------------------------
-- 
--                                   swSSO
-- 
--        SSO Windows et Web avec Internet Explorer, Firefox, Mozilla...
-- 
--                 Copyright (C) 2004-2015 - Sylvain WERDEFROY
-- 
-- 							 http://www.swsso.fr
--                    
--                              sylvain@swsso.fr
-- 
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

DROP TABLE STATS;

CREATE TABLE IF NOT EXISTS `stats` (
  `shausername` varchar(40) NOT NULL,
  `logindate` varchar(8) NOT NULL,
  `nconfigs` int(11) NOT NULL,
  `nsso` int(11) NOT NULL,
  `nenrolled` int(11) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
