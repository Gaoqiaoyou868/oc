from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.firefox.firefox_profile import FirefoxProfile
from selenium.webdriver.common.by import By
from selenium.webdriver.common.keys import Keys
from selenium.webdriver.common.action_chains import ActionChains
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

print("=== Setting up Firefox with geckodriver ===")
gecko_path = os.path.join(os.environ['TEMP'], 'geckodriver', 'geckodriver.exe')
print(f"geckodriver: {gecko_path}")

options = Options()
options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
options.add_argument("--width=1920")
options.add_argument("--height=1080")
options.set_preference("dom.webdriver.enabled", False)
options.set_preference("useAutomationExtension", False)

service = Service(
    executable_path=gecko_path,
    service_args=["--log", "trace"],
    service_log_path=os.path.join(output_dir, "geckodriver.log")
)

driver = webdriver.Firefox(
    service=service,
    options=options
)

def ss(name):
    path = os.path.join(output_dir, name)
    try:
        driver.save_screenshot(path)
        print(f"  [SS] {name}")
    except:
        print(f"  [FAIL] screenshot {name}")

try:
    print("\n[1] Opening drawDB.app editor...")
    driver.get("https://www.drawdb.app/editor")
    time.sleep(8)
    ss("01_loaded.png")
    print(f"  Title: {driver.title}")
    
    # Dump page source for debugging
    with open(os.path.join(output_dir, "page_debug.html"), "w", encoding="utf-8") as f:
        f.write(driver.page_source)
    print("  Page source dumped")
    
    # Try clicking on Import - look at navigation/toolbar area
    print("\n[2] Finding import/sql menu...")
    
    # We'll use keyboard shortcut Ctrl+Shift+I if it exists
    actions = ActionChains(driver)
    actions.key_down(Keys.CONTROL).key_down(Keys.SHIFT).send_keys('I').key_up(Keys.SHIFT).key_up(Keys.CONTROL).perform()
    time.sleep(2)
    ss("02_tried_shortcut.png")
    
    # Try File menu approach - look for top bar buttons
    all_elements = driver.find_elements(By.CSS_SELECTOR, 
        "button, [role='button'], [role='menuitem'], [class*='menu'], a")
    
    print(f"  Total interactive elements: {len(all_elements)}")
    
    # Look specifically for Import
    import_btn = None
    for el in all_elements:
        try:
            text = el.text.strip().lower()
            if text in ['import', 'sql', 'file', '导入']:
                print(f"  Found element: '{el.text}' ({el.tag_name})")
                if el.is_displayed():
                    import_btn = el
                    break
        except:
            pass
    
    if import_btn:
        driver.execute_script("arguments[0].click();", import_btn)
        print(f"  Clicked: '{import_btn.text}'")
        time.sleep(2)
        ss("03_after_import_click.png")
    else:
        print("  No Import button found by text, trying first toolbar button...")
        # Try clicking the first few toolbar buttons
        toolbar_els = driver.find_elements(By.CSS_SELECTOR, "nav button, header button, [class*=toolbar] button")
        print(f"  Toolbar buttons: {len(toolbar_els)}")
        for tb in toolbar_els[:3]:
            try:
                if tb.is_displayed():
                    print(f"  Toolbar: '{tb.text}'")
                    driver.execute_script("arguments[0].click();", tb)
                    time.sleep(1)
                    break
            except:
                pass
        ss("03_after_toolbar_click.png")
    
    # Wait and check for dialog/modal
    time.sleep(2)
    
    # Check for textareas
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"\n[3] Textareas found: {len(textareas)}")
    
    for i, ta in enumerate(textareas):
        try:
            if ta.is_displayed():
                print(f"  Textarea {i}: visible, size={ta.size}")
                ta.click()
                time.sleep(0.5)
                # Use JavaScript to set value
                sql_escaped = sql_content.replace('\\', '\\\\').replace("'", "\\'").replace('\n', '\\n')
                driver.execute_script("arguments[0].value = arguments[1]; arguments[0].dispatchEvent(new Event('input', { bubbles: true }));", ta, sql_content)
                time.sleep(2)
                print(f"  Pasted {len(sql_content)} chars via JS")
                ss("04_after_paste.png")
                break
        except Exception as e:
            print(f"  Error with textarea {i}: {e}")
    
    # Click Import button to confirm
    time.sleep(2)
    all_btns = driver.find_elements(By.TAG_NAME, "button")
    import_confirm = False
    for btn in all_btns:
        try:
            if btn.is_displayed() and btn.text.strip().lower() in ['import', 'import sql', '生成', '确认', 'submit']:
                driver.execute_script("arguments[0].click();", btn)
                print(f"  Confirmed: '{btn.text}'")
                import_confirm = True
                time.sleep(3)
                ss("05_after_import.png")
                break
        except:
            pass
    
    if not import_confirm:
        print("  No confirm button, trying footer/submit buttons...")
        for btn in all_btns:
            try:
                txt = btn.text.strip().lower()
                if txt and btn.is_displayed():
                    print(f"  Visible button: '{btn.text}'")
                    if len(btn.text.strip()) > 0:
                        driver.execute_script("arguments[0].click();", btn)
                        time.sleep(3)
                        ss("05_clicked_submit.png")
                        break
            except:
                pass
    
    print("\n[4] Waiting for diagram to render...")
    time.sleep(5)
    ss("06_diagram_initial.png")
    
    # Try to export as image
    time.sleep(3)
    ss("07_diagram_final.png")
    
    print("\n=== Automation Complete ===")
    print(f"Screenshots saved to: {output_dir}")

except Exception as e:
    print(f"\nError: {e}")
    import traceback
    traceback.print_exc()
    try:
        ss("error.png")
    except:
        pass
finally:
    time.sleep(1)
    driver.quit()
    print("Browser closed.")
