from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
import time
import os

output_dir = r'C:\Users\17537\Desktop\oc\数据库'

sql_content = """CREATE TABLE teams (
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

def ss(name):
    path = os.path.join(output_dir, name)
    try:
        driver.save_screenshot(path)
        print(f"  [SS] {name}")
    except:
        pass

def find_click(text_pattern, tag="button"):
    els = driver.find_elements(By.XPATH, f"//{tag}[contains(text(), '{text_pattern}')]")
    for el in els:
        try:
            if el.is_displayed():
                driver.execute_script("arguments[0].click();", el)
                return True
        except:
            pass
    return False

print("=== drawDB Automation ===")
gecko_path = os.path.join(os.environ['TEMP'], 'geckodriver', 'geckodriver.exe')

options = Options()
options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
options.add_argument("--width=1920")
options.add_argument("--height=1080")
options.set_preference("dom.webdriver.enabled", False)
options.set_preference("useAutomationExtension", False)

service = Service(executable_path=gecko_path)
driver = webdriver.Firefox(service=service, options=options)

try:
    print("\n[1] Opening drawDB editor...")
    driver.get("https://www.drawdb.app/editor")
    time.sleep(8)
    ss("01_editor_loaded.png")

    # Step 1: Dialog "Choose a database" should already be open
    print("\n[2] Choosing MySQL from database dialog...")
    time.sleep(2)
    
    # Click on MySQL option in the dialog
    mysql_divs = driver.find_elements(By.XPATH, "//*[contains(text(), 'MySQL')]")
    print(f"  MySQL elements: {len(mysql_divs)}")
    
    for div in mysql_divs:
        try:
            if div.is_displayed():
                print(f"  Clicking MySQL option: '{div.text}' (tag={div.tag_name})")
                driver.execute_script("arguments[0].click();", div)
                time.sleep(1)
                ss("02_mysql_selected.png")
                break
        except:
            pass

    # Click Confirm button
    print("\n[3] Clicking Confirm...")
    confirm_btn = driver.find_elements(By.XPATH, "//*[contains(text(), 'Confirm')]")
    for btn in confirm_btn:
        try:
            if btn.is_displayed() and btn.is_enabled():
                print(f"  Clicking Confirm: '{btn.text}'")
                driver.execute_script("arguments[0].click();", btn)
                time.sleep(3)
                ss("03_after_confirm.png")
                break
        except:
            pass

    # Try clicking the confirm using aria-label
    confirm_aria = driver.find_elements(By.CSS_SELECTOR, "[aria-label='confirm']")
    for btn in confirm_aria:
        try:
            if btn.is_displayed():
                print(f"  Found confirm by aria-label, disabled={btn.get_attribute('disabled')}")
                if not btn.get_attribute('disabled'):
                    driver.execute_script("arguments[0].click();", btn)
                    time.sleep(3)
                    ss("03b_confirm_aria.png")
        except:
            pass

    # Step 2: SQL code editor should appear
    print("\n[4] Looking for SQL code editor / textarea...")
    time.sleep(3)
    
    # Try finding a CodeMirror or similar editor
    editors = driver.find_elements(By.CSS_SELECTOR, 
        "textarea, [contenteditable='true'], .CodeMirror, .monaco-editor, [class*=code], [class*=editor]")
    print(f"  Found {len(editors)} editor elements")
    
    pasted = False
    for ed in editors:
        try:
            if ed.is_displayed():
                print(f"  Editor: tag={ed.tag_name}, class={ed.get_attribute('class')}")
                ed.click()
                time.sleep(1)
                # Try pasting SQL
                ed.send_keys(sql_content)
                print(f"  Pasted {len(sql_content)} chars")
                pasted = True
                ss("04_sql_pasted.png")
                break
        except:
            pass

    if not pasted:
        # Try clicking in the center of the dialog body to focus the editor
        print("  Trying body click to focus editor...")
        body = driver.find_element(By.TAG_NAME, "body")
        body.click()
        time.sleep(1)
        actions = ActionChains(driver)
        actions.send_keys(sql_content)
        actions.perform()
        ss("04_sql_pasted_via_body.png")

    # Step 3: Click Import/Generate button
    print("\n[5] Looking for Import/Generate button...")
    time.sleep(2)
    for text in ['Import', 'Generate', 'import', 'Submit']:
        found = find_click(text)
        if found:
            print(f"  Clicked: {text}")
            time.sleep(3)
            ss("05_after_import.png")
            break

    # Step 4: Wait for diagram to render
    print("\n[6] Waiting for ER diagram...")
    time.sleep(5)
    ss("06_diagram.png")
    
    # Try to arrange/auto-layout
    print("\n[7] Trying auto-layout...")
    for text in ['Layout', 'Auto', 'Arrange', 'auto']:
        found = find_click(text)
        if found:
            print(f"  Clicked: {text}")
            time.sleep(3)
            break

    ss("07_diagram_arranged.png")
    time.sleep(2)
    ss("08_diagram_final.png")

    print("\n=== Automation Complete ===")

except Exception as e:
    print(f"\nError: {e}")
    import traceback
    traceback.print_exc()
    try:
        ss("error.png")
    except:
        pass
finally:
    time.sleep(2)
    driver.quit()
    print("Browser closed.")
