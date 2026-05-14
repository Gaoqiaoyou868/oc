from selenium import webdriver
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

print("Starting Firefox with Marionette...")
firefox_options = Options()
firefox_options.binary_location = r'C:\Program Files\Mozilla Firefox\firefox.exe'
firefox_options.add_argument("--width=1920")
firefox_options.add_argument("--height=1080")
# Enable marionette protocol (built into Firefox)
firefox_options.set_preference("marionette", True)
firefox_options.set_preference("dom.webdriver.enabled", False)
firefox_options.set_preference("useAutomationExtension", False)
firefox_options.log.level = "trace"

# Use Firefox's built-in driver capability
driver = webdriver.Firefox(options=firefox_options)

try:
    print("Opening drawDB.app...")
    driver.get("https://www.drawdb.app/")
    time.sleep(8)
    
    # Screenshot initial state
    driver.save_screenshot(os.path.join(output_dir, "01_loaded.png"))
    print("Screenshot 1: Page loaded")
    print(f"Page title: {driver.title}")
    print(f"URL: {driver.current_url}")
    
    # Save the page source for analysis
    with open(os.path.join(output_dir, "page_source.html"), "w", encoding="utf-8") as f:
        f.write(driver.page_source)
    print("Page source saved")
    
    # Take a full-page screenshot using the body element
    body = driver.find_element(By.TAG_NAME, "body")
    body.screenshot(os.path.join(output_dir, "02_body.png"))
    print("Body screenshot done")
    
    # Look for all buttons and their text
    all_buttons = driver.find_elements(By.TAG_NAME, "button")
    print(f"\nFound {len(all_buttons)} buttons:")
    for btn in all_buttons:
        try:
            txt = btn.text.strip()
            if txt:
                print(f"  Button: '{txt}' display={btn.is_displayed()}")
        except:
            pass
    
    # Look for all links/anchors
    all_links = driver.find_elements(By.TAG_NAME, "a")
    print(f"\nFound {len(all_links)} links:")
    for link in all_links:
        try:
            txt = link.text.strip()
            if txt:
                print(f"  Link: '{txt}' href='{link.get_attribute('href')}'")
        except:
            pass
    
    # Look for elements with role="button"
    role_btns = driver.find_elements(By.CSS_SELECTOR, "[role='button']")
    print(f"\nFound {len(role_btns)} role=button elements:")
    for btn in role_btns:
        try:
            txt = btn.text.strip()
            if txt:
                print(f"  '{txt}'")
        except:
            pass
    
    # Try to find any input/textarea or code editor
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"\nFound {len(textareas)} textareas")
    
    inputs = driver.find_elements(By.TAG_NAME, "input")
    print(f"Found {len(inputs)} inputs")
    
    # Try to find the canvas/svg where the diagram is rendered
    canvases = driver.find_elements(By.TAG_NAME, "canvas")
    print(f"Found {len(canvases)} canvases")
    
    svgs = driver.find_elements(By.TAG_NAME, "svg")
    print(f"Found {len(svgs)} svgs")
    
    # Try to find any monaco editor or code editor elements
    editors = driver.find_elements(By.CSS_SELECTOR, "[class*=monaco], [class*=editor], [class*=code], [class*=sql]")
    print(f"Found {len(editors)} editor-like elements")
    
    print("\nDone with analysis.")
    
except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
    try:
        driver.save_screenshot(os.path.join(output_dir, "error.png"))
    except:
        pass
finally:
    time.sleep(2)
    driver.quit()
    print("Browser closed.")
