--
--    This file is part of SourceOP.
--
--    SourceOP is free software: you can redistribute it and/or modify
--    it under the terms of the GNU General Public License as published by
--    the Free Software Foundation, either version 3 of the License, or
--    (at your option) any later version.
--
--    SourceOP is distributed in the hope that it will be useful,
--    but WITHOUT ANY WARRANTY; without even the implied warranty of
--    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--    GNU General Public License for more details.
--
--    You should have received a copy of the GNU General Public License
--    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
--

-- SourceOP plugin database structure

-- --------------------------------------------------------

-- 
-- Table structure for table `connectlog`
-- 

CREATE TABLE `connectlog` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `connect` enum('C','D') NOT NULL default 'C',
  `serverip` varchar(32) NOT NULL default '',
  `mapname` varchar(255) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `steamid` varchar(24) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `ip` varchar(32) NOT NULL default '',
  `timestamp` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `steamid` (`steamid`),
  KEY `ip` (`ip`),
  KEY `timestamp` (`timestamp`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `ipbans`
-- 

CREATE TABLE `ipbans` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `banenable` enum('Y','N') NOT NULL default 'Y',
  `name` varchar(64) NOT NULL default '',
  `steamid` varchar(32) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `ip` varchar(16) NOT NULL default '',
  `bannername` varchar(64) NOT NULL default '',
  `bannerid` varchar(32) NOT NULL default '',
  `serverip` varchar(32) NOT NULL default '',
  `map` varchar(255) NOT NULL default '',
  `reason` varchar(255) NOT NULL default '',
  `timestamp` int(11) unsigned NOT NULL default '0',
  `expiration` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `banenable` (`banenable`),
  KEY `ip` (`ip`),
  KEY `expiration` (`expiration`),
  KEY `bannerid` (`bannerid`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `playernames`
-- 

CREATE TABLE `playernames` (
  `nameid` int(11) unsigned NOT NULL auto_increment,
  `steamid` varchar(32) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `firstused` int(11) unsigned NOT NULL default '0',
  `lastused` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`nameid`),
  UNIQUE KEY `steamid` (`steamid`,`name`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `players`
-- 

CREATE TABLE `players` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `steamid` varchar(32) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `name` varchar(64) NOT NULL default '',
  `ip` varchar(32) NOT NULL default '',
  `firstseen` int(11) unsigned NOT NULL default '0',
  `lastseen` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `steamid` (`steamid`),
  KEY `ip` (`ip`),
  KEY `name` (`name`),
  KEY `profileid` (`profileid`),
  KEY `lastseen` (`lastseen`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `rslots`
-- 

CREATE TABLE `rslots` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `steamid` varchar(24) NOT NULL default '',
  `name` varchar(32) NOT NULL default '',
  `ip1` varbinary(4) NOT NULL default '\0\0\0\0',
  `ip2` varbinary(4) NOT NULL default '\0\0\0\0',
  `ip3` varbinary(4) NOT NULL default '\0\0\0\0',
  `donated` double NOT NULL default '0',
  `timestamp` int(11) unsigned NOT NULL default '0',
  `lastupdate` int(11) unsigned NOT NULL default '0',
  `expiration` int(11) unsigned NOT NULL default '4294967295',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `steamid` (`steamid`),
  KEY `profileid` (`profileid`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `sessions`
-- 

CREATE TABLE `sessions` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `serverip` varchar(32) NOT NULL default '',
  `mapname` varchar(255) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `steamid` varchar(24) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `ip` varchar(32) NOT NULL default '',
  `connecttime` bigint(20) NOT NULL default '0',
  `disconnecttime` bigint(20) NOT NULL default '0',
  `sessiontime` bigint(20) NOT NULL default '0',
  `timestamp` bigint(20) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `steamid` (`steamid`),
  KEY `profileid` (`profileid`),
  KEY `ip` (`ip`),
  KEY `sessiontime` (`sessiontime`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `specialitems`
-- 

CREATE TABLE `specialitems` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `owner` bigint(20) unsigned NOT NULL default '0',
  `active` enum('Y','N') NOT NULL default 'Y',
  `itemdef` int(10) unsigned NOT NULL default '0',
  `petname` varchar(128) NOT NULL default '',
  `equipped` tinyint(4) NOT NULL default '0',
  `level` int(10) unsigned NOT NULL default '0',
  `quality` int(10) unsigned NOT NULL default '0',
  `attributes` int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `owner` (`owner`,`itemdef`),
  KEY `equipped` (`equipped`),
  KEY `active` (`active`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `specialitems_attributes`
-- 

CREATE TABLE `specialitems_attributes` (
  `id` int(10) unsigned NOT NULL auto_increment,
  `itemid` int(10) unsigned NOT NULL default '0',
  `owner` bigint(20) unsigned NOT NULL default '0',
  `attribdef` int(10) unsigned NOT NULL default '0',
  `value` varchar(32) NOT NULL default '',
  PRIMARY KEY  (`id`),
  KEY `itemid` (`itemid`,`owner`,`attribdef`)
) ENGINE=MyISAM;

-- --------------------------------------------------------

-- 
-- Table structure for table `steamidbans`
-- 

CREATE TABLE `steamidbans` (
  `id` int(11) unsigned NOT NULL auto_increment,
  `banenable` enum('Y','N') NOT NULL default 'Y',
  `name` varchar(64) NOT NULL default '',
  `steamid` varchar(32) NOT NULL default '',
  `profileid` bigint(20) unsigned NOT NULL default '0',
  `ip` varchar(16) NOT NULL default '',
  `bannername` varchar(64) NOT NULL default '',
  `bannerid` varchar(32) NOT NULL default '',
  `serverip` varchar(32) NOT NULL default '',
  `map` varchar(255) NOT NULL default '',
  `reason` varchar(255) NOT NULL default '',
  `extrainfo` text NOT NULL,
  `timestamp` int(11) unsigned NOT NULL default '0',
  `expiration` int(11) unsigned NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `banenable` (`banenable`),
  KEY `steamid` (`steamid`),
  KEY `expiration` (`expiration`),
  KEY `bannerid` (`bannerid`)
) ENGINE=MyISAM;