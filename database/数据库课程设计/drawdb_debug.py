from selenium import webdriver
from selenium.webdriver.firefox.service import Service
from selenium.webdriver.firefox.options import Options
from selenium.webdriver.common.by import By
import time
import os

output_dir = r'C:\Users\17537\Desktop\oc\数据库'

def ss(name):
    path = os.path.join(output_dir, name)
    try:
        driver.save_screenshot(path)
        print(f"  [SS] {name}")
    except:
        pass

def dump_source(name):
    path = os.path.join(output_dir, name)
    with open(path, "w", encoding="utf-8") as f:
        f.write(driver.page_source)
    print(f"  [HTML] {name}")

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
    driver.get("https://www.drawdb.app/editor")
    time.sleep(8)
    dump_source("step0_initial.html")
    ss("step0.png")

    # Find and click MySQL option in dialog
    mysql = driver.find_elements(By.XPATH, "//*[contains(text(), 'MySQL')]")
    print(f"MySQL elements: {len(mysql)}")
    for m in mysql:
        if m.is_displayed():
            print(f"  Found: tag={m.tag_name}, class={m.get_attribute('class')}")
            driver.execute_script("arguments[0].click();", m)
            time.sleep(1)
            ss("step1_mysql_clicked.png")
            dump_source("step1_mysql_clicked.html")
            break

    # Click Confirm
    confirm = driver.find_elements(By.XPATH, "//*[contains(text(), 'Confirm')]")
    for c in confirm:
        if c.is_displayed():
            print(f"Confirm: tag={c.tag_name}, disabled={c.get_attribute('disabled')}")
            if not c.get_attribute('disabled'):
                driver.execute_script("arguments[0].click();", c)
                time.sleep(4)
                ss("step2_confirm_clicked.png")
                dump_source("step2_confirm_clicked.html")
                break

    # Now dump ALL elements recursively to find textareas, inputs, code editors
    print("\n=== Full DOM element dump ===")
    all_elements = driver.find_elements(By.CSS_SELECTOR, "*")
    print(f"Total elements: {len(all_elements)}")
    
    textareas = driver.find_elements(By.TAG_NAME, "textarea")
    print(f"Textareas: {len(textareas)}")
    for i, ta in enumerate(textareas):
        print(f"  TA[{i}]: displayed={ta.is_displayed()}, loc={ta.location}, size={ta.size}")
    
    inputs = driver.find_elements(By.TAG_NAME, "input")
    print(f"Inputs: {len(inputs)}")
    for i, inp in enumerate(inputs):
        if inp.is_displayed():
            print(f"  Input[{i}]: type={inp.get_attribute('type')}, placeholder={inp.get_attribute('placeholder')}")
    
    # Look for code editors
    for sel in ["[class*=code]", "[class*=Code]", "[class*=editor]", "[class*=Editor]", 
                "[class*=monaco]", "[class*=Monaco]", "[class*=cm-]", ".cm-editor",
                "[contenteditable=true]", "[role=textbox]", "[class*=sql]", "[class*=SQL]"]:
        els = driver.find_elements(By.CSS_SELECTOR, sel)
        for el in els:
            if el.is_displayed():
                print(f"  Editor-like: sel='{sel}', tag={el.tag_name}, class={el.get_attribute('class')[:80]}")

    # Look for modal/dialog content
    modals = driver.find_elements(By.CSS_SELECTOR, "[class*=modal], [class*=dialog], [role=dialog]")
    print(f"Modals: {len(modals)}")
    for m in modals:
        if m.is_displayed():
            text = m.text[:300]
            print(f"  Modal text: {text}")

    # Try to check the semi-modal structure  
    semi_modal = driver.find_elements(By.CSS_SELECTOR, ".semi-modal, [class*=semi-modal]")
    print(f"Semi modals: {len(semi_modal)}")

    ss("step3_dump.png")

except Exception as e:
    print(f"Error: {e}")
    import traceback
    traceback.print_exc()
finally:
    time.sleep(1)
    driver.quit()
