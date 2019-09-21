CREATE TABLE players (
	playerid BIGSERIAL NOT NULL,
	username VARCHAR(64) NOT NULL,
	email VARCHAR(64) NOT NULL,
	suspended SMALLINT NOT NULL,
	CONSTRAINT players_PK PRIMARY KEY (playerid)
);

CREATE TABLE player_keys (
        playerid BIGINT NOT NULL,
        public_key BYTEA NOT NULL,
        not_before TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
        not_after TIMESTAMP,
        CONSTRAINT player_keys_PK PRIMARY KEY (playerid),
        CONSTRAINT player_keys_FK FOREIGN KEY (playerid) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE characters (
	characterid BIGSERIAL NOT NULL,
	owner BIGINT NOT NULL,
	charactername VARCHAR(64),
	CONSTRAINT characters_PK PRIMARY KEY (characterid),
        CONSTRAINT characters_FK FOREIGN KEY (owner) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE servers (
	serverid BIGSERIAL NOT NULL,
	ip VARCHAR(45) NOT NULL,
	port SMALLINT NOT NULL,
	owner BIGINT NOT NULL,
        servername VARCHAR(64),
	CONSTRAINT servers_PK PRIMARY KEY (serverid),
        CONSTRAINT servers_FK FOREIGN KEY (owner) REFERENCES players(playerid) ON UPDATE CASCADE ON DELETE NO ACTION
);

CREATE TABLE server_access (
	serverid BIGINT NOT NULL,
	characterid BIGINT NOT NULL,
	access_type SMALLINT NOT NULL DEFAULT 2,
	CONSTRAINT server_access_PK PRIMARY KEY (serverid, characterid),
        CONSTRAINT server_access_server_FK FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
        CONSTRAINT server_access_character_FK FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE
);

CREATE TABLE geometries (
  geometryid BIGSERIAL NOT NULL,
  geometryname VARCHAR(64) NOT NULL,
  CONSTRAINT geometries_PK PRIMARY KEY (geometryid)
);

CREATE TABLE server_objects (
  objectid BIGSERIAL NOT NULL,
  serverid BIGINT NOT NULL,
  characterid BIGINT NOT NULL DEFAULT 0,
  geometryid BIGINT NOT NULL DEFAULT 0,
  sector_x BIGINT NOT NULL,
  sector_y BIGINT NOT NULL,
  sector_z BIGINT NOT NULL,
  pos_x BIGINT NOT NULL,
  pos_y BIGINT NOT NULL,
  pos_z BIGINT NOT NULL,
  CONSTRAINT server_objects_PK PRIMARY KEY (serverid, objectid, characterid),
  CONSTRAINT server_objects_server_FK FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT server_objects_character_FK FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE,
  CONSTRAINT server_objects_geometry_FK FOREIGN KEY (geometryid) REFERENCES geometries(geometryid) ON UPDATE CASCADE ON DELETE SET DEFAULT
);

CREATE TABLE skills (
	skillid BIGINT NOT NULL,
	skillname VARCHAR(64) NOT NULL,
	CONSTRAINT skills_PK PRIMARY KEY (skillid)
);

CREATE TABLE server_skills (
	serverid BIGINT NOT NULL,
	skillid BIGINT NOT NULL,
	defaultid BIGINT NOT NULL DEFAULT 0,
	lower SMALLINT NOT NULL,
	upper SMALLINT NOT NULL,
	CONSTRAINT server_skills_PK PRIMARY KEY (serverid, skillid),
        CONSTRAINT server_skills_server_FK FOREIGN KEY (serverid) REFERENCES servers(serverid) ON UPDATE CASCADE ON DELETE CASCADE,
        CONSTRAINT server_skills_skill_FK FOREIGN KEY (skillid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE CASCADE,
        CONSTRAINT server_skills_default_FK FOREIGN KEY (defaultid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE SET DEFAULT
);

CREATE TABLE character_skills (
	characterid BIGINT NOT NULL,
	skillid BIGINT NOT NULL,
	level SMALLINT NOT NULL DEFAULT 0,
	improvement SMALLINT NOT NULL DEFAULT 0,
        last_increase TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
	CONSTRAINT character_skills_PK PRIMARY KEY (characterid, skillid),
        CONSTRAINT character_skills_character_FK FOREIGN KEY (characterid) REFERENCES characters(characterid) ON UPDATE CASCADE ON DELETE CASCADE,
        CONSTRAINT character_skills_skill_FK FOREIGN KEY (skillid) REFERENCES skills(skillid) ON UPDATE CASCADE ON DELETE CASCADE
);

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
