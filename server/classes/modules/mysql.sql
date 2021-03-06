CREATE TABLE players (
  playerid BIGINT NOT NULL AUTO_INCREMENT,
  username VARCHAR(64) NOT NULL,
  email VARCHAR(64) NOT NULL,
  suspended TINYINT NOT NULL DEFAULT 0,
  PRIMARY KEY (playerid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE player_keys (
  playerid BIGINT NOT NULL,
  public_key BLOB NOT NULL,
  not_before DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  not_after DATETIME,
  PRIMARY KEY (playerid),
  FOREIGN KEY (playerid) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE CASCADE
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

CREATE TABLE geometries (
  geometryid BIGINT NOT NULL AUTO_INCREMENT,
  geometryname VARCHAR(64) NOT NULL,
  PRIMARY KEY (geometryid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE server_objects (
  objectid BIGINT NOT NULL AUTO_INCREMENT,
  serverid BIGINT NOT NULL,
  characterid BIGINT NOT NULL DEFAULT 0,
  geometryid BIGINT NOT NULL DEFAULT 0,
  sector_x BIGINT NOT NULL,
  sector_y BIGINT NOT NULL,
  sector_z BIGINT NOT NULL,
  pos_x BIGINT NOT NULL,
  pos_y BIGINT NOT NULL,
  pos_z BIGINT NOT NULL,
  PRIMARY KEY (serverid, objectid, characterid),
  FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (geometryid) REFERENCES geometries(geometryid) ON UPDATE CASCADE ON DELETE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE skills (
  skillid BIGINT NOT NULL,
  skillname VARCHAR(64) NOT NULL,
  PRIMARY KEY (skillid)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE server_skills (
  serverid BIGINT NOT NULL,
  skillid BIGINT NOT NULL,
  defaultid BIGINT NOT NULL DEFAULT 0,
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
  level SMALLINT NOT NULL DEFAULT 0,
  improvement SMALLINT NOT NULL DEFAULT 0,
  last_increase TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (characterid, skillid),
  FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE,
  FOREIGN KEY (skillid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

INSERT INTO players (playerid, username, email, suspended) VALUES (0, 'No player', 'No email', 0);
INSERT INTO characters (characterid, owner, charactername) VALUES (0, 0, 'No character');
INSERT INTO servers (serverid, ip, port, owner, servername) VALUES (0, '0', 0, 0, 'No server');
INSERT INTO geometries (geometryid, geometryname) VALUES (0, 'No geometry');
INSERT INTO skills (skillid, skillname) VALUES (0, 'No skill');
INSERT INTO skills (skillid, skillname) VALUES (1, 'Control');
INSERT INTO skills (skillid, skillname) VALUES (2, 'Uncontrol');
INSERT INTO skills (skillid, skillname) VALUES (3, 'Move');
INSERT INTO skills (skillid, skillname) VALUES (4, 'Rotate');
INSERT INTO skills (skillid, skillname) VALUES (5, 'Stop');
