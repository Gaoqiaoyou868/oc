-- =============================================
-- 方案A：3表结构（对应用户已有实验报告）
-- =============================================

CREATE TABLE teams (
    team_id INT PRIMARY KEY AUTO_INCREMENT,
    team_name VARCHAR(100) NOT NULL
);

CREATE TABLE players (
    player_id INT PRIMARY KEY AUTO_INCREMENT,
    player_name VARCHAR(100) NOT NULL,
    team_id INT,
    FOREIGN KEY (team_id) REFERENCES teams(team_id) ON DELETE SET NULL
);

CREATE TABLE player_season_stats (
    stat_id INT PRIMARY KEY AUTO_INCREMENT,
    player_id INT,
    season VARCHAR(50),
    matches_played INT,
    minutes_played INT,
    goals INT,
    assists INT,
    shots INT,
    passes_completed INT,
    tackles INT,
    rating FLOAT,
    FOREIGN KEY (player_id) REFERENCES players(player_id) ON DELETE CASCADE
);


-- =============================================
-- 方案B：5表结构（改进版，推荐，用于实验一完整设计）
-- =============================================

CREATE TABLE teams (
    team_id INT PRIMARY KEY AUTO_INCREMENT,
    team_name VARCHAR(100) NOT NULL,
    city VARCHAR(50),
    stadium VARCHAR(100)
);

CREATE TABLE seasons (
    season_id INT PRIMARY KEY AUTO_INCREMENT,
    year INT NOT NULL
);

CREATE TABLE players (
    player_id INT PRIMARY KEY AUTO_INCREMENT,
    player_name VARCHAR(100) NOT NULL,
    nationality VARCHAR(50),
    position VARCHAR(20),
    birth_date DATE,
    team_id INT,
    FOREIGN KEY (team_id) REFERENCES teams(team_id) ON DELETE SET NULL
);

CREATE TABLE matches (
    match_id INT PRIMARY KEY AUTO_INCREMENT,
    season_id INT NOT NULL,
    match_number INT,
    match_date DATE,
    home_team_id INT,
    away_team_id INT,
    FOREIGN KEY (season_id) REFERENCES seasons(season_id) ON DELETE CASCADE,
    FOREIGN KEY (home_team_id) REFERENCES teams(team_id) ON DELETE CASCADE,
    FOREIGN KEY (away_team_id) REFERENCES teams(team_id) ON DELETE CASCADE
);

CREATE TABLE performances (
    performance_id INT PRIMARY KEY AUTO_INCREMENT,
    player_id INT NOT NULL,
    match_id INT NOT NULL,
    minutes_played INT DEFAULT 0,
    goals INT DEFAULT 0,
    assists INT DEFAULT 0,
    shots INT DEFAULT 0,
    passes_completed INT DEFAULT 0,
    tackles INT DEFAULT 0,
    rating DECIMAL(4,2) DEFAULT 0.00,
    FOREIGN KEY (player_id) REFERENCES players(player_id) ON DELETE CASCADE,
    FOREIGN KEY (match_id) REFERENCES matches(match_id) ON DELETE CASCADE
);
