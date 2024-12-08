import { test, expect } from '@playwright/test';

test('has title', async ({ page }) => {
  await page.goto('/');
  // Expect a title "to contain" a substring.
  await expect(page).toHaveTitle(/pepp/i);
});

test('loads wasm container', async ({ page }) => {
  await page.goto('/');
  // Ensure that local storage is cleared to force "what's new" pane to appear.
  await page.evaluate(() => window.localStorage.clear());
  await page.evaluate(() => window.sessionStorage.clear());
  const shadow_root = page.locator("#qt-shadow-container")
  test.setTimeout(100_000);
  await shadow_root.waitFor()
  await page.mouse.click(100, 200)
  await page.waitForTimeout(100);
});
