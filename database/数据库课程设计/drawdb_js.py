from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
import time
import os
import re

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

    # Use JavaScript to find and click the MySQL option properly
    print("[1] Selecting MySQL via JavaScript...")
    
    driver.execute_script("""
        // Find all divs containing MySQL text in the modal
        const allDivs = document.querySelectorAll('div');
        let mysqlDiv = null;
        for (const div of allDivs) {
            if (div.textContent.trim() === 'MySQL' && div.offsetParent !== null) {
                mysqlDiv = div;
                break;
            }
        }
        if (mysqlDiv) {
            // Click the PARENT container that has the click handler
            const container = mysqlDiv.closest('.space-y-3') || mysqlDiv.parentElement;
            container.click();
            // Also dispatch click event
            const event = new MouseEvent('click', { bubbles: true, cancelable: true });
            container.dispatchEvent(event);
            return 'Clicked: ' + (container.className || container.tagName);
        }
        return 'MySQL div not found';
    """)
    time.sleep(2)
    ss("02_mysql_clicked_js.png")

    # Check if confirm button is now enabled
    confirm_status = driver.execute_script("""
        const confirmBtn = document.querySelector('[aria-label="confirm"]');
        if (confirmBtn) {
            return 'disabled=' + confirmBtn.disabled + ', aria-disabled=' + confirmBtn.getAttribute('aria-disabled');
        }
        return 'not found';
    """)
    print(f"  Confirm status: {confirm_status}")

    # Try clicking confirm via JS
    driver.execute_script("""
        const confirmBtn = document.querySelector('[aria-label="confirm"]');
        if (confirmBtn && !confirmBtn.disabled) {
            confirmBtn.click();
            return 'clicked';
        }
        // Force enable and click
        if (confirmBtn) {
            confirmBtn.removeAttribute('disabled');
            confirmBtn.removeAttribute('aria-disabled');
            confirmBtn.classList.remove('semi-button-disabled', 'semi-button-primary-disabled');
            confirmBtn.disabled = false;
            confirmBtn.click();
            return 'force clicked';
        }
        return 'not found';
    """)
    time.sleep(3)
    ss("03_confirm_clicked.png")
    print("  Confirm action executed")

    # After confirm, a SQL import dialog should appear
    # Look for any kind of code editor
    time.sleep(3)
    
    # Dump all visible text to understand what's on screen
    body_text = driver.find_element(By.TAG_NAME, "body").text
    print(f"\n[2] Body text preview: {body_text[:500]}")

    # Look for the SQL import dialog/editor
    dr = driver.execute_script("""
        // Find all visible dialog/modal elements
        const dialogs = document.querySelectorAll('[class*="modal"], [class*="dialog"], [role="dialog"]');
        const results = [];
        for (const d of dialogs) {
            if (d.offsetParent !== null) {
                results.push({
                    tag: d.tagName,
                    class: d.className.substring(0, 100),
                    text: d.textContent.substring(0, 200),
                    visible: d.offsetParent !== null
                });
            }
        }
        return results;
    """)
    print(f"\n  Dialogs found: {len(dr)}")
    for d in dr[:5]:
        print(f"  Dialog: {d['class']}")
        print(f"    Text: {d['text']}")

    # Try to find a textarea, code editor, or any paste-able element
    paste_target = driver.execute_script("""
        const textareas = document.querySelectorAll('textarea');
        for (const ta of textareas) {
            if (ta.offsetParent !== null) return 'textarea found';
        }
        const inputs = document.querySelectorAll('input[type="text"]');
        for (const inp of inputs) {
            if (inp.offsetParent !== null) return 'input found';
        }
        // Look for the monaco editor or codemirror
        const editors = document.querySelectorAll('.monaco-editor, .CodeMirror, [class*="cm-editor"]');
        for (const ed of editors) {
            if (ed.offsetParent !== null) return 'code editor found: ' + ed.className.substring(0, 80);
        }
        // Look for any contenteditable
        const contentEditable = document.querySelectorAll('[contenteditable="true"]');
        for (const ce of contentEditable) {
            if (ce.offsetParent !== null) return 'contenteditable found';
        }
        return 'nothing found';
    """)
    print(f"\n  Paste target: {paste_target}")

    # If the SQL dialog is open, try to interact with it
    # Let's try to find the code editor surface within the semi-modal
    time.sleep(2)
    ss("04_after_confirm.png")

    # Try using keyboard to paste SQL
    # The code editor might be a contenteditable div or a hidden textarea that needs to be focused
    print("\n[3] Trying to find and focus the editor...")
    
    focused = driver.execute_script("""
        // Focus any visible textarea
        const textareas = document.querySelectorAll('textarea');
        for (const ta of textareas) {
            if (ta.offsetParent !== null) {
                ta.focus();
                return 'textarea focused';
            }
        }
        // Try to find the editor content area
        const allDivs = document.querySelectorAll('div[class*="cm-content"], .monaco-editor .view-lines');
        for (const div of allDivs) {
            if (div.offsetParent !== null) {
                div.focus();
                return 'editor content focused';
            }
        }
        // Try semi-portal content
        const portals = document.querySelectorAll('.semi-portal');
        for (const p of portals) {
            // Find inputs inside
            const input = p.querySelector('textarea, input, [contenteditable]');
            if (input && input.offsetParent !== null) {
                input.focus();
                return 'portal input focused: ' + input.tagName;
            }
        }
        return 'nothing to focus';
    """)
    print(f"  Focus result: {focused}")

    # Try pasting SQL via JS
    print("\n[4] Pasting SQL via JavaScript...")
    paste_result = driver.execute_script(f"""
        const textareas = document.querySelectorAll('textarea');
        for (const ta of textareas) {{
            if (ta.offsetParent !== null) {{
                const nativeInputValueSetter = Object.getOwnPropertyDescriptor(
                    window.HTMLTextAreaElement.prototype, 'value'
                ).set;
                nativeInputValueSetter.call(ta, arguments[0]);
                ta.dispatchEvent(new Event('input', {{ bubbles: true }}));
                ta.dispatchEvent(new Event('change', {{ bubbles: true }}));
                return 'pasted to textarea: ' + ta.value.length + ' chars';
            }}
        }}
        // Try React/controlled input approach
        const inputs = document.querySelectorAll('input');
        for (const inp of inputs) {{
            if (inp.offsetParent !== null) {{
                const nativeSetter = Object.getOwnPropertyDescriptor(
                    window.HTMLInputElement.prototype, 'value'
                ).set;
                nativeSetter.call(inp, arguments[0]);
                inp.dispatchEvent(new Event('input', {{ bubbles: true }}));
                return 'pasted to input';
            }}
        }}
        return 'no paste target';
    """, sql_content)
    print(f"  Paste result: {paste_result}")
    ss("05_after_paste.png")

    # Try clicking Import button
    print("\n[5] Looking for Import button...")
    import_result = driver.execute_script("""
        const buttons = document.querySelectorAll('button');
        for (const btn of buttons) {
            if (btn.offsetParent !== null && btn.textContent.trim().toLowerCase() === 'import') {
                btn.click();
                return 'clicked import';
            }
        }
        // Look for any button containing 'import'
        for (const btn of buttons) {
            if (btn.offsetParent !== null && btn.textContent.toLowerCase().includes('import')) {
                btn.click();
                return 'clicked: ' + btn.textContent.trim();
            }
        }
        return 'no import button';
    """)
    print(f"  Import: {import_result}")
    time.sleep(5)
    ss("06_after_import.png")

    # Check what's visible now
    time.sleep(3)
    ss("07_final.png")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
finally:
    time.sleep(1)
    driver.quit()
    print("Done")
