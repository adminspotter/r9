CREATE TABLE players (
  playerid BIGINT NOT NULL AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL,
  password VARCHAR(128) NOT NULL,
  email VARCHAR(64) NOT NULL,
  suspended TINYINT NOT NULL DEFAULT 0,
  PRIMARY KEY (playerid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE characters (
  characterid BIGINT NOT NULL AUTO_INCREMENT,
  owner BIGINT NOT NULL,
  charactername VARCHAR(64),
  PRIMARY KEY (characterid),
  FOREIGN KEY (owner) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE servers (
  serverid BIGINT NOT NULL AUTO_INCREMENT,
  ip VARCHAR(45) NOT NULL,
  port SMALLINT UNSIGNED NOT NULL,
  owner BIGINT NOT NULL,
  servername VARCHAR(64),
  PRIMARY KEY (serverid),
  FOREIGN KEY (owner) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE server_access (
  serverid BIGINT NOT NULL,
  characterid BIGINT NOT NULL,
  access_type TINYINT NOT NULL DEFAULT 2,
  PRIMARY KEY (serverid, characterid),
  FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE player_logins (
  playerid BIGINT NOT NULL,
  characterid BIGINT NOT NULL,
  serverid BIGINT NOT NULL,
  src_ip INTEGER NOT NULL,
  src_port SMALLINT NOT NULL,
  login_time DATETIME NOT NULL,
  enter_time DATETIME,
  exit_time DATETIME,
  logout_time DATETIME,
  PRIMARY KEY (playerid, characterid, login_time),
  FOREIGN KEY (playerid) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE NO ACTION,
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE NO ACTION,
  FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE skills (
  skillid BIGINT NOT NULL,
  skillname VARCHAR(64) NOT NULL,
  PRIMARY KEY (skillid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE server_skills (
  serverid BIGINT NOT NULL,
  skillid BIGINT NOT NULL,
  defaultid BIGINT NOT NULL,
  lower SMALLINT NOT NULL,
  upper SMALLINT NOT NULL,
  PRIMARY KEY (serverid, skillid),
  FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (skillid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (defaultid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE character_skills (
  characterid BIGINT NOT NULL,
  skillid BIGINT NOT NULL,
  level SMALLINT NOT NULL,
  improvement SMALLINT NOT NULL,
  last_increase DATETIME,
  PRIMARY KEY (characterid, skillid),
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (skillid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE server_characters (
  serverid BIGINT NOT NULL,
  characterid BIGINT NOT NULL,
  objectid BIGINT NOT NULL,
  sector_x BIGINT NOT NULL,
  sector_y BIGINT NOT NULL,
  sector_z BIGINT NOT NULL,
  pos_x BIGINT NOT NULL,
  pos_y BIGINT NOT NULL,
  pos_z BIGINT NOT NULL,
  PRIMARY KEY (serverid, characterid),
  FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE NO ACTION,
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
