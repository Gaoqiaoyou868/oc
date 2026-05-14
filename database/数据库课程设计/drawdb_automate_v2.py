from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.support import expected_conditions as EC
from selenium.common.exceptions import TimeoutException, NoSuchElementException
import time
import os
import base64

output_dir = r'C:\Users\17537\Desktop\oc\数据库'

sql_content = """-- Premier League Player Performance DB
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

print("Starting Firefox automation for drawDB...")

firefox_options = Options()
firefox_options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
firefox_options.add_argument("--width=1920")
firefox_options.add_argument("--height=1080")

gecko_path = r'C:\Users\17537\AppData\Roaming\npm\node_modules\geckodriver\dist\geckodriver.exe'
if not os.path.exists(gecko_path):
    gecko_path = r'C:\Users\17537\AppData\Roaming\npm\geckodriver'
    
print(f"Using geckodriver: {gecko_path}")

service = Service(executable_path=gecko_path)
driver = webdriver.Firefox(service=service, options=firefox_options)

try:
    print("Opening drawDB.app...")
    driver.get("https://www.drawdb.app/")
    time.sleep(5)
    
    # Take initial screenshot
    driver.save_screenshot(os.path.join(output_dir, "01_drawdb_loaded.png"))
    print("[1/5] Page loaded, screenshot saved")
    
    # Press keyboard shortcut for import (Shift+Ctrl+I or similar)
    from selenium.webdriver.common.keys import Keys
    from selenium.webdriver.common.action_chains import ActionChains
    
    # Try to find the import button - look for various elements
    found = False
    
    # Method 1: Look for toolbar buttons
    for selector in ["[data-testid]", "button", "[role='button']", "nav button", ".toolbar button"]:
        try:
            elements = driver.find_elements(By.CSS_SELECTOR, selector)
            for el in elements:
                text = el.text.strip().lower()
                if text in ('import', 'file', 'sql', '打开', '导入'):
                    print(f"  Found element: '{el.text}' ({el.tag_name})")
                    if el.is_displayed():
                        el.click()
                        time.sleep(2)
                        found = True
                        break
        except:
            pass
        if found:
            break
    
    if not found:
        # Method 2: Try menu/file approach - look for hamburger menu or file menu
        try:
            # Try clicking the top-left menu/file button
            top_buttons = driver.find_elements(By.CSS_SELECTOR, "header button, [class*=menu], [class*=file], [class*=toolbar] button")
            for btn in top_buttons:
                if btn.is_displayed():
                    print(f"  Trying top button: '{btn.text}'")
                    btn.click()
                    time.sleep(1)
                    # Now look for Import option in the dropdown
                    dropdown_items = driver.find_elements(By.CSS_SELECTOR, "[role=menuitem], li, [class*=menu] [class*=item]")
                    for item in dropdown_items:
                        if 'import' in item.text.lower() or 'sql' in item.text.lower():
                            print(f"  Found import option: '{item.text}'")
                            item.click()
                            time.sleep(2)
                            found = True
                            break
                    if found:
                        break
        except:
            pass
    
    if not found:
        # Method 3: Just try to find any clickable area with Import text
        try:
            import_link = driver.find_element(By.XPATH, "//*[contains(text(), 'Import') or contains(text(), 'SQL')]")
            if import_link.is_displayed():
                print(f"  Found import via XPath: '{import_link.text}'")
                import_link.click()
                time.sleep(2)
                found = True
        except:
            pass
    
    # Take screenshot after clicking import
    driver.save_screenshot(os.path.join(output_dir, "02_after_import_click.png"))
    print(f"[2/5] After import click, found={found}")
    
    # Now look for a textarea to paste SQL
    time.sleep(2)
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"  Found {len(textareas)} textareas")
    
    pasted = False
    for ta in textareas:
        try:
            if ta.is_displayed():
                ta.clear()
                ta.send_keys(sql_content)
                print(f"  Pasted SQL into textarea! ({len(sql_content)} chars)")
                pasted = True
                time.sleep(1)
                break
        except:
            pass
    
    driver.save_screenshot(os.path.join(output_dir, "03_after_sql_paste.png"))
    print(f"[3/5] After SQL paste, pasted={pasted}")
    
    if pasted:
        # Look for confirm/import button in the dialog
        try:
            confirm_btns = driver.find_elements(By.XPATH, "//*[contains(text(), 'Import') or contains(text(), 'Generate') or contains(text(), 'Submit') or contains(text(), 'Confirm') or contains(text(), 'OK')]")
            for btn in confirm_btns:
                if btn.is_displayed():
                    print(f"  Clicking confirm button: '{btn.text}'")
                    btn.click()
                    time.sleep(3)
                    break
        except:
            pass
        
        # Also try looking for any bottom/submit button
        try:
            all_btns = driver.find_elements(By.TAG_NAME, "button")
            for btn in all_btns:
                if btn.is_displayed() and btn.text.strip():
                    bt = btn.text.strip().lower()
                    if bt in ('import', 'generate', 'submit', 'confirm', 'ok', '导入', '生成'):
                        print(f"  Clicking button: '{btn.text}'")
                        btn.click()
                        time.sleep(3)
                        break
        except:
            pass
    
    # Wait for diagram to render
    time.sleep(5)
    
    # Take screenshot of the final diagram
    driver.save_screenshot(os.path.join(output_dir, "04_drawdb_diagram.png"))
    print("[4/5] Diagram screenshot saved")
    
    # Also try to get a full page screenshot using the canvas/svg
    time.sleep(2)
    driver.save_screenshot(os.path.join(output_dir, "05_drawdb_final.png"))
    print("[5/5] Final screenshot saved")
    
    # Save page HTML for debugging
    with open(os.path.join(output_dir, "page.html"), "w", encoding="utf-8") as f:
        f.write(driver.page_source)
    print("Page source saved for debugging")
    
    print("\nAll done! Check the screenshots in the oc folder.")
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
    try:
        driver.save_screenshot(os.path.join(output_dir, "error_screenshot.png"))
        print("Error screenshot saved")
    except:
        pass
finally:
    time.sleep(2)
    driver.quit()
    print("Browser closed.")
