from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from webdriver_manager.firefox import GeckoDriverManager
import time
import os

output_dir = r'C:\Users\17537\Desktop\oc\数据库'

sql_content = """-- 英超球员 2019-2023 数据管理系统
-- 导入drawDB自动生成ER图

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
);"""

print("Setting up Firefox driver...")
firefox_options = Options()
# Use non-headless mode so it actually renders
firefox_options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
firefox_options.set_preference("dom.webdriver.enabled", False)
firefox_options.set_preference("useAutomationExtension", False)
# Set window size for good screenshot
firefox_options.add_argument("--width=1920")
firefox_options.add_argument("--height=1080")

try:
    service = Service(GeckoDriverManager().install())
    driver = webdriver.Firefox(service=service, options=firefox_options)
    print("Firefox launched!")
    
    # Navigate to drawDB
    print("Opening drawDB.app...")
    driver.get("https://www.drawdb.app/")
    
    # Wait for page to load
    time.sleep(5)
    
    # Take initial screenshot
    driver.save_screenshot(os.path.join(output_dir, "drawdb_initial.png"))
    print("Initial screenshot saved")
    
    # Try to find the Import button/option
    # Look for various possible selectors
    try:
        # Method 1: Look for a button/menu with "Import" text
        import_btn = WebDriverWait(driver, 10).until(
            EC.element_to_be_clickable((By.XPATH, "//*[contains(text(), 'Import')]"))
        )
        print(f"Found Import element: {import_btn.tag_name} - {import_btn.text}")
        import_btn.click()
        time.sleep(2)
    except:
        print("Could not find Import button via text")
    
    # Method 2: Try using keyboard shortcut or menu
    try:
        # Look for sidebar/file menu
        menu_btns = driver.find_elements(By.TAG_NAME, "button")
        for btn in menu_btns:
            if 'import' in btn.text.lower() or 'file' in btn.text.lower():
                print(f"Found button: {btn.text}")
                btn.click()
                time.sleep(1)
                break
    except:
        pass
    
    # Method 3: Try looking for the SQL import dialog
    try:
        textareas = driver.find_elements(By.TAG_NAME, "textarea")
        if textareas:
            print(f"Found {len(textareas)} textareas")
            for ta in textareas:
                if ta.is_displayed():
                    ta.send_keys(sql_content)
                    print("SQL pasted into textarea!")
                    time.sleep(1)
                    # Look for import/confirm button
                    import_confirm = driver.find_elements(By.XPATH, "//*[contains(text(), 'Import') or contains(text(), 'import')]")
                    for btn in import_confirm:
                        if btn.is_displayed():
                            btn.click()
                            print("Clicked Import confirm!")
                            break
                    break
    except:
        pass
    
    # Wait for diagram to generate
    time.sleep(5)
    
    # Take screenshot of the diagram
    driver.save_screenshot(os.path.join(output_dir, "drawdb_er_diagram.png"))
    print("Final ER diagram screenshot saved!")
    
    # Get page source for debugging
    with open(os.path.join(output_dir, "page_source.html"), "w", encoding="utf-8") as f:
        f.write(driver.page_source)
    print("Page source saved")
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
finally:
    try:
        driver.quit()
    except:
        pass
    print("Done!")
