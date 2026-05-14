from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
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

def ss(name):
    path = os.path.join(output_dir, name)
    driver.save_screenshot(path)
    print(f"  [SS] {name}")

gecko_path = os.path.join(os.environ['TEMP'], 'geckodriver', 'geckodriver.exe')
options = Options()
options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
options.add_argument("--width=1920")
options.add_argument("--height=1080")

service = Service(executable_path=gecko_path)
driver = webdriver.Firefox(service=service, options=options)

try:
    driver.get("https://www.drawdb.app/editor")
    time.sleep(8)
    ss("01_loaded.png")

    # Step 1: Use ActionChains to properly click the MySQL option
    print("[1] Clicking MySQL using ActionChains...")
    mysql_container = driver.find_element(By.XPATH, 
        "//div[contains(@class, 'space-y-3') and .//div[contains(text(), 'MySQL')]]")
    
    actions = ActionChains(driver)
    actions.move_to_element(mysql_container).click().perform()
    time.sleep(2)
    ss("02_mysql_clicked.png")

    # Verify confirm button state
    confirm_btn = driver.find_element(By.CSS_SELECTOR, "[aria-label='confirm']")
    disabled = confirm_btn.get_attribute('disabled')
    print(f"  Confirm disabled: {disabled}")

    # If still disabled, try clicking the inner div specifically
    if disabled:
        print("  MySQL click didn't register, trying inner div...")
        mysql_inner = driver.find_element(By.XPATH, "//div[contains(text(), 'MySQL') and contains(@class, 'font-semibold')]")
        actions = ActionChains(driver)
        actions.move_to_element(mysql_inner).click().perform()
        time.sleep(2)
        ss("02b_mysql_inner_clicked.png")
        disabled = confirm_btn.get_attribute('disabled')
        print(f"  Confirm disabled after inner click: {disabled}")

    # If still disabled, try clicking the parent container
    if disabled:
        print("  Trying parent container click...")
        driver.execute_script("""
            const div = document.querySelector('.grid-cols-3 > div:first-child');
            div.click();
            ['click', 'mousedown', 'mouseup'].forEach(type => {
                div.dispatchEvent(new MouseEvent(type, { bubbles: true, cancelable: true, view: window }));
            });
        """)
        time.sleep(2)
        disabled = confirm_btn.get_attribute('disabled')
        print(f"  Confirm disabled after parent click: {disabled}")

    # If still disabled, force enable the button
    if disabled:
        print("  Forcing confirm button enable...")
        driver.execute_script("""
            const btn = document.querySelector('[aria-label="confirm"]');
            btn.disabled = false;
            btn.removeAttribute('disabled');
            btn.removeAttribute('aria-disabled');
            btn.classList.remove('semi-button-disabled', 'semi-button-primary-disabled');
        """)

    # Click Confirm
    print("\n[2] Clicking Confirm...")
    driver.execute_script("document.querySelector('[aria-label=\"confirm\"]').click()")
    time.sleep(4)
    ss("03_confirm_clicked.png")

    # Now look for the SQL import dialog
    body_text = driver.find_element(By.TAG_NAME, "body").text
    print(f"\n[3] Body text: {body_text[:300]}")

    # Look for textareas
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"  Textareas: {len(textareas)}")
    for i, ta in enumerate(textareas):
        if ta.is_displayed():
            print(f"  TA[{i}]: visible, trying to paste...")
            ta.send_keys(sql_content)
            print(f"  Pasted!")
            ss("04_pasted.png")
            
            # Click Import button
            import_btns = driver.find_elements(By.XPATH, "//button[contains(text(), 'Import')]")
            for ib in import_btns:
                if ib.is_displayed():
                    ib.click()
                    print(f"  Clicked Import!")
                    time.sleep(4)
                    ss("05_imported.png")
                    break
            break
    
    # If no textarea, try finding CodeMirror
    if len([ta for ta in driver.find_elements(By.TAG_NAME, "textarea") if ta.is_displayed()]) == 0:
        print("  No visible textarea, looking for code editor...")
        driver.execute_script("""
            const all = document.querySelectorAll('*');
            for (const el of all) {
                if (el.offsetParent !== null) {
                    const style = getComputedStyle(el);
                    if (el.tagName === 'TEXTAREA' || 
                        (style.userSelect !== 'none' && el.isContentEditable)) {
                        console.log('Found:', el.tagName, el.className);
                    }
                }
            }
        """)

    ss("06_final_state.png")
    print("\n=== Done ===")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
finally:
    time.sleep(1)
    driver.quit()
