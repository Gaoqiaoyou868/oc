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
        return True
    except:
        return False

print("=== Setting up Firefox ===")
gecko_path = os.path.join(os.environ['TEMP'], 'geckodriver', 'geckodriver.exe')

options = Options()
options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
options.add_argument("--width=1920")
options.add_argument("--height=1080")
options.set_preference("dom.webdriver.enabled", False)
options.set_preference("useAutomationExtension", False)
options.set_preference("intl.accept_languages", "en-US")

service = Service(executable_path=gecko_path)

driver = webdriver.Firefox(service=service, options=options)
wait = WebDriverWait(driver, 15)

try:
    print("\n[1] Opening drawDB editor...")
    driver.get("https://www.drawdb.app/editor")
    time.sleep(10)
    ss("01_editor_loaded.png")
    print(f"  URL: {driver.current_url}")
    print(f"  Title: {driver.title}")

    # Save page source
    with open(os.path.join(output_dir, "editor_page.html"), "w", encoding="utf-8") as f:
        f.write(driver.page_source)

    print("\n[2] Looking for Import/SQL functionality...")

    # Method 1: Try the top menu bar - look for File menu
    all_buttons = driver.find_elements(By.TAG_NAME, "button")
    print(f"  Total buttons: {len(all_buttons)}")
    for btn in all_buttons:
        try:
            txt = btn.text.strip()
            aria = btn.get_attribute("aria-label") or ""
            title_attr = btn.get_attribute("title") or ""
            if txt or aria or title_attr:
                print(f"    Button: '{txt}' aria='{aria}' title='{title_attr}'")
        except:
            pass

    # Method 2: Try keyboard shortcut Ctrl+I or Ctrl+Shift+I
    print("\n[3] Trying keyboard shortcut for Import...")
    actions = ActionChains(driver)
    actions.key_down(Keys.ALT).send_keys('f').key_up(Keys.ALT).perform()
    time.sleep(1)
    actions.send_keys('i').perform()
    time.sleep(2)
    ss("03_keyboard_menu.png")

    # Try Ctrl+Shift+I
    actions2 = ActionChains(driver)
    actions2.key_down(Keys.CONTROL).key_down(Keys.SHIFT).send_keys('i').key_up(Keys.SHIFT).key_up(Keys.CONTROL).perform()
    time.sleep(2)
    ss("03b_ctrl_shift_i.png")

    # Method 3: Look for import in the SVG/icons - check all elements
    print("\n[4] Finding import/SQL textareas...")
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"  Textareas: {len(textareas)}")
    for i, ta in enumerate(textareas):
        try:
            if ta.is_displayed():
                print(f"  TA {i}: displayed")
        except:
            pass

    # Method 4: Try to access all semantic elements
    all_els = driver.find_elements(By.CSS_SELECTOR, 
        "[class*=Import], [class*=import], [class*=sql], [class*=SQL], [class*=menu], [aria-label*=import]")
    print(f"  Import-related elements: {len(all_els)}")
    for el in all_els:
        try:
            print(f"    '{el.tag_name}' text='{el.text[:50]}'")
        except:
            pass

    # Method 5: Try clicking various menu areas
    print("\n[5] Trying to find and trigger import...")
    # Look for a toolbar/menu bar at the top
    top_bar = driver.find_elements(By.CSS_SELECTOR, 
        "header, nav, [class*=toolbar], [class*=navbar], [class*=menubar], [role=menubar]")
    print(f"  Top bars: {len(top_bar)}")
    
    # Look for "File" menu text
    file_els = driver.find_elements(By.XPATH, "//*[contains(text(), 'File') or contains(text(), 'file')]")
    print(f"  File menu elements: {len(file_els)}")
    for el in file_els:
        try:
            if el.is_displayed():
                print(f"    '{el.text}' tag={el.tag_name}")
                driver.execute_script("arguments[0].click();", el)
                time.sleep(2)
                ss("05_file_menu_clicked.png")
                
                # Now look for Import option in dropdown
                import_items = driver.find_elements(By.XPATH, 
                    "//*[contains(text(), 'Import') or contains(text(), 'import') or contains(text(), 'SQL')]")
                print(f"    Import items after click: {len(import_items)}")
                for item in import_items:
                    try:
                        if item.is_displayed():
                            print(f"      Clicking: '{item.text}'")
                            driver.execute_script("arguments[0].click();", item)
                            time.sleep(2)
                            ss("05_import_option_clicked.png")
                            break
                    except:
                        pass
                break
        except:
            pass

    # Check for textarea/modal after import
    time.sleep(3)
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"\n[6] Textareas after import click: {len(textareas)}")
    for i, ta in enumerate(textareas):
        try:
            if ta.is_displayed():
                print(f"  TA {i}: visible at ({ta.location['x']}, {ta.location['y']})")
                ta.click()
                time.sleep(0.5)
                # Clear and paste SQL
                ta.clear()
                time.sleep(0.3)
                ta.send_keys(sql_content)
                print(f"  Pasted {len(sql_content)} chars!")
                ss("06_sql_pasted.png")
                time.sleep(2)
                
                # Look for import/confirm button
                confirm_btns = driver.find_elements(By.XPATH, 
                    "//*[contains(text(), 'Import') or contains(text(), 'import') or contains(text(), 'Submit') or contains(text(), 'Generate')]")
                for cbtn in confirm_btns:
                    try:
                        if cbtn.is_displayed():
                            print(f"  Clicking confirm: '{cbtn.text}'")
                            driver.execute_script("arguments[0].click();", cbtn)
                            time.sleep(3)
                            ss("07_after_import_confirm.png")
                            break
                    except:
                        pass
                break
        except Exception as e:
            print(f"  TA {i} error: {e}")

    # Wait for diagram to render
    print("\n[7] Waiting for diagram...")
    time.sleep(5)
    ss("08_diagram.png")
    time.sleep(3)
    ss("09_diagram_final.png")

    print("\n=== Done ===")

except Exception as e:
    print(f"\nError: {e}")
    import traceback
    traceback.print_exc()
    ss("error.png")
finally:
    time.sleep(1)
    driver.quit()
    print("Browser closed.")
