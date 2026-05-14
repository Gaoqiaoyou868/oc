from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.common.action_chains import ActionChains
from selenium.webdriver.common.keys import Keys
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
    # Step 1: Open editor and handle initial dialog
    print("[1] Opening editor...")
    driver.get("https://www.drawdb.app/editor")
    time.sleep(6)

    # The "Choose a database" dialog is open - select MySQL and confirm
    print("  Selecting MySQL and closing setup dialog...")
    driver.execute_script("""
        // Click MySQL option container
        const containers = document.querySelectorAll('.grid-cols-3 > div');
        if (containers.length > 0) containers[0].click();
        // Enable and click confirm
        setTimeout(() => {
            const btn = document.querySelector('[aria-label="confirm"]');
            if (btn) {
                btn.disabled = false;
                btn.removeAttribute('disabled');
                btn.classList.remove('semi-button-disabled');
                btn.click();
            }
        }, 300);
    """)
    time.sleep(4)
    ss("01_editor_ready.png")

    # Step 2: Click File menu → Import → MySQL
    print("\n[2] Navigating: File → Import → MySQL...")
    
    # Click "File" menu
    file_menu = driver.find_element(By.XPATH, "//*[contains(text(), 'File')]")
    file_menu.click()
    time.sleep(2)
    ss("02_file_menu_open.png")

    # Click "Import" submenu
    import_els = driver.find_elements(By.XPATH, "//*[contains(text(), 'Import')]")
    print(f"  Import elements: {len(import_els)}")
    for el in import_els:
        if el.is_displayed():
            print(f"  Clicking: '{el.text}'")
            el.click()
            time.sleep(2)
            ss("03_import_submenu.png")
            break

    # Click MySQL from the import options
    mysql_items = driver.find_elements(By.XPATH, "//*[contains(text(), 'MySQL')]")
    print(f"  MySQL items: {len(mysql_items)}")
    for el in mysql_items:
        if el.is_displayed():
            print(f"  Clicking: '{el.text}'")
            el.click()
            time.sleep(3)
            ss("04_mysql_selected.png")
            break

    # Step 3: SQL import dialog should now be open with a code editor
    print("\n[3] Looking for SQL code editor...")
    time.sleep(3)
    
    # Check if there's an import dialog
    body_text = driver.find_element(By.TAG_NAME, "body").text
    print(f"  Body: {body_text[:400]}")

    # Look for textarea or code editor
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"  Textareas: {len(textareas)}")

    # Try to find any input element
    all_inputs = driver.execute_script("""
        const inputs = [];
        document.querySelectorAll('textarea').forEach(el => {
            if (el.offsetParent !== null) {
                inputs.push({tag: 'textarea', visible: true});
            }
        });
        document.querySelectorAll('input').forEach(el => {
            if (el.offsetParent !== null) {
                inputs.push({tag: 'input.' + el.type, visible: true});
            }
        });
        document.querySelectorAll('[contenteditable="true"]').forEach(el => {
            if (el.offsetParent !== null) {
                inputs.push({tag: 'contenteditable', class: el.className.substring(0,50), visible: true});
            }
        });
        return inputs;
    """)
    print(f"  Visible inputs: {len(all_inputs)}")
    for inp in all_inputs:
        print(f"    {inp}")

    # Check for semi-modal with SQL editor
    modals = driver.execute_script("""
        return Array.from(document.querySelectorAll('.semi-modal, .semi-portal')).map(el => ({
            tag: el.tagName,
            class: el.className.substring(0, 80),
            visible: el.offsetParent !== null,
            text: el.textContent.substring(0, 300)
        }));
    """)
    print(f"\n  Modals: {len(modals)}")
    for m in modals:
        if m['visible']:
            print(f"  Modal: {m['text']}")

    # If no dialog, the import might have happened via a different UI
    # Try looking for any import-related dialog
    ss("05_after_import_select.png")

    # Try to find the code editor/paste area
    print("\n[4] Trying to paste SQL...")
    pasted = False
    
    for ta in driver.find_elements(By.TAG_NAME, "textarea"):
        try:
            if ta.is_displayed():
                ta.clear()
                ta.send_keys(sql_content)
                print(f"  Pasted to textarea!")
                pasted = True
                ss("06_sql_pasted.png")
                break
        except:
            pass

    if not pasted:
        # Try clicking in the center of the screen to focus
        print("  Clicking body to try focusing editor...")
        body = driver.find_element(By.TAG_NAME, "body")
        body.click()
        time.sleep(1)
        ActionChains(driver).send_keys(sql_content).perform()
        ss("06_sql_pasted_body.png")

    # Look for Import button in the dialog and click it
    print("\n[5] Looking for Import button...")
    for text in ['Import', 'import']:
        btns = driver.find_elements(By.XPATH, f"//button[contains(text(), '{text}')]")
        for btn in btns:
            if btn.is_displayed():
                print(f"  Clicking: '{btn.text}'")
                btn.click()
                time.sleep(4)
                ss("07_diagram_generated.png")
                break

    time.sleep(5)
    ss("08_final.png")
    print("\n=== Done ===")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
    ss("error.png")
finally:
    time.sleep(2)
    driver.quit()
