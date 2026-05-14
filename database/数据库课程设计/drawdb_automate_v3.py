from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException, NoSuchElementException
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

sql_content_3table = """CREATE TABLE teams (
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
);"""

print("=== drawDB Firefox Automation ===")

gecko_path = os.path.join(os.environ['TEMP'], 'geckodriver', 'geckodriver.exe')
print(f"Using geckodriver: {gecko_path}")

firefox_options = Options()
firefox_options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
firefox_options.add_argument("--width=1920")
firefox_options.add_argument("--height=1080")

service = Service(executable_path=gecko_path)
driver = webdriver.Firefox(service=service, options=firefox_options)

def screenshot(name):
    path = os.path.join(output_dir, name)
    driver.save_screenshot(path)
    print(f"  Screenshot: {name}")

try:
    print("\n[1] Opening drawDB.app...")
    driver.get("https://www.drawdb.app/")
    time.sleep(8)
    screenshot("drawdb_01_loaded.png")
    print(f"  Title: {driver.title}")

    print("\n[2] Analyzing page elements...")
    btns = driver.find_elements(By.TAG_NAME, "button")
    print(f"  Buttons found: {len(btns)}")
    for btn in btns:
        try:
            txt = btn.text.strip()
            if txt:
                print(f"    '{txt}'")
        except:
            pass

    print("\n[3] Looking for Import button...")
    import_found = False
    for btn in btns:
        try:
            txt = btn.text.strip().lower()
            if txt in ('import', 'sql', '导入'):
                print(f"  Clicking: '{btn.text}'")
                driver.execute_script("arguments[0].click();", btn)
                time.sleep(3)
                import_found = True
                screenshot("drawdb_02_after_import_click.png")
                break
        except:
            pass

    if not import_found:
        print("  Import button not found by text, trying keyboard shortcut...")
        from selenium.webdriver.common.keys import Keys
        actions = webdriver.ActionChains(driver)
        actions.key_down(Keys.CONTROL).key_down(Keys.SHIFT).send_keys('i').key_up(Keys.SHIFT).key_up(Keys.CONTROL).perform()
        # or try Alt+I
        time.sleep(3)
        import_found = True
        screenshot("drawdb_02_keyboard_shortcut.png")

    print("\n[4] Looking for textarea/code editor to paste SQL...")
    time.sleep(2)
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"  Textareas: {len(textareas)}")
    
    # Also look for monaco editor or code editor
    code_editors = driver.find_elements(By.CSS_SELECTOR, 
        "[class*='monaco'], [class*='code'], [class*='editor'], [contenteditable='true'], .cm-editor, .CodeMirror")
    print(f"  Code editors: {len(code_editors)}")

    pasted = False
    # Try pasting into textareas
    for ta in textareas:
        try:
            if ta.is_displayed():
                driver.execute_script("arguments[0].scrollIntoView(true);", ta)
                time.sleep(0.5)
                ta.clear()
                time.sleep(0.5)
                ta.send_keys(sql_content)
                print(f"  Pasted into textarea ({len(sql_content)} chars)")
                pasted = True
                time.sleep(2)
                screenshot("drawdb_03_after_paste.png")
                break
        except:
            pass

    if not pasted:
        # Try clicking on code editor then pasting
        for editor in code_editors:
            try:
                if editor.is_displayed():
                    editor.click()
                    time.sleep(1)
                    # Try to find input element inside
                    input_elements = editor.find_elements(By.CSS_SELECTOR, 
                        "textarea, input, [role='textbox'], [contenteditable='true']")
                    for inp in input_elements:
                        if inp.is_displayed():
                            inp.send_keys(sql_content)
                            pasted = True
                            print(f"  Pasted into editor sub-element")
                            time.sleep(2)
                            screenshot("drawdb_03_after_paste2.png")
                            break
                    if pasted:
                        break
            except:
                pass

    # Last resort: use JavaScript to find and fill any visible input
    if not pasted:
        js_check = driver.execute_script("""
            const inputs = document.querySelectorAll('textarea, [contenteditable="true"], [role="textbox"]');
            for (const el of inputs) {
                if (el.offsetParent !== null) {
                    return el.tagName + ' ' + el.className;
                }
            }
            return 'none found';
        """)
        print(f"  JS check for inputs: {js_check}")
        
        # Try selecting from the context menu or dialog
        # Look for any open modal/dialog
        modals = driver.find_elements(By.CSS_SELECTOR, 
            "[class*='modal'], [class*='dialog'], [class*='overlay'], [class*='popup']")
        print(f"  Modals/dialogs: {len(modals)}")
        for modal in modals:
            if modal.is_displayed():
                print(f"  Dialog found: class={modal.get_attribute('class')}")
                # Find all text inside
                text_content = modal.text[:500]
                print(f"  Dialog text: {text_content}")

    print("\n[5] Looking for Import/Confirm button...")
    time.sleep(2)
    confirm_found = False
    all_btns = driver.find_elements(By.TAG_NAME, "button")
    for btn in all_btns:
        try:
            txt = btn.text.strip().lower()
            if txt in ('import', 'generate', 'submit', 'confirm', 'ok', '导入', '生成', 'confirm import'):
                print(f"  Clicking confirm: '{btn.text}'")
                driver.execute_script("arguments[0].click();", btn)
                time.sleep(3)
                confirm_found = True
                screenshot("drawdb_04_after_confirm.png")
                break
        except:
            pass

    if not confirm_found:
        print("  No confirm button found, trying any visible button in dialog...")
        for btn in all_btns:
            try:
                if btn.is_displayed():
                    driver.execute_script("arguments[0].click();", btn)
                    time.sleep(3)
                    screenshot("drawdb_04_clicked_random.png")
                    break
            except:
                pass

    print("\n[6] Waiting for ER diagram to render...")
    time.sleep(5)
    screenshot("drawdb_05_diagram.png")
    
    time.sleep(3)
    screenshot("drawdb_06_final.png")

    print("\n=== Done! ===")
    print(f"Check screenshots in: {output_dir}")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
    try:
        screenshot("drawdb_error.png")
    except:
        pass
finally:
    time.sleep(2)
    driver.quit()
    print("Browser closed.")
