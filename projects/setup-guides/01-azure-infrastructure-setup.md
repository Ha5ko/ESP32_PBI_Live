# Azure Infrastructure Setup Guide

Follow these steps in order. You'll need values from earlier steps in later steps.

---

## Step 1: Gather Your Power BI Report Details

You'll need the **Group ID** (workspace) and **Report ID** from your published report.

1. Open your report in **app.powerbi.com**
2. Look at the URL — it has this format:
   ```
   https://app.powerbi.com/groups/XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX/reports/YYYYYYYY-YYYY-YYYY-YYYY-YYYYYYYYYYYY/...
   ```
3. Copy and save:
   - **Group ID** = the first GUID (after `/groups/`)
   - **Report ID** = the second GUID (after `/reports/`)
4. Also note the **page name** — click the page tab you want to export, and look at the URL for `pageName=PageXXXXXX` or find it in the report settings. If your report only has one page, you can skip this for now.

**Save these values:**
```
GROUP_ID  = xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
REPORT_ID = yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
PAGE_NAME = (optional, will use default page if omitted)
```

---

## Step 2: Azure AD App Registration (Service Principal)

This creates a "service account" that the Azure Function uses to call the Power BI API.

### 2a: Register the App

1. Go to **Azure Portal** → **Microsoft Entra ID** (formerly Azure AD) → **App registrations**
2. Click **+ New registration**
3. Settings:
   - **Name**: `PBI-Dashboard-ESP32` (or whatever you like)
   - **Supported account types**: "Accounts in this organizational directory only"
   - **Redirect URI**: leave blank
4. Click **Register**
5. On the app's overview page, copy:
   - **Application (client) ID**
   - **Directory (tenant) ID**

### 2b: Create a Client Secret

1. In the app registration, go to **Certificates & secrets**
2. Click **+ New client secret**
3. Description: `esp32-dashboard`
4. Expiry: choose 12 or 24 months
5. Click **Add**
6. **Copy the secret Value immediately** — it won't be shown again

### 2c: Add Power BI API Permissions

1. In the app registration, go to **API permissions**
2. Click **+ Add a permission**
3. Select **Power BI Service**
4. Select **Application permissions** (not Delegated)
5. Check **Report.Read.All**
6. Click **Add permissions**
7. Click **Grant admin consent for [your org]** (you may need a Global Admin to do this)
8. Confirm the status shows ✅ "Granted for [org]"

**Save these values:**
```
TENANT_ID     = xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
CLIENT_ID     = xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
CLIENT_SECRET = xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

---

## Step 3: Enable Service Principal Access in Power BI

The Power BI admin must allow service principals to use the API.

### 3a: Power BI Admin Setting

1. Go to **app.powerbi.com** → **Settings** (gear icon) → **Admin portal**
2. Go to **Tenant settings**
3. Scroll to **Developer settings** section
4. Find **"Allow service principals to use Power BI APIs"**
5. Enable it
6. Under "Apply to:", choose either:
   - **The entire organization**, OR
   - **Specific security groups** — if so, create a security group in Entra ID, add your app registration to it, and specify that group here
7. Click **Apply**

Note: This setting can take up to 15 minutes to propagate.

### 3b: Add Service Principal to the Workspace

1. Go to **app.powerbi.com** → open the **workspace** containing your report
2. Click **Access** (or Manage access)
3. Add the app by searching for the name you used (e.g., `PBI-Dashboard-ESP32`)
4. Set the role to **Viewer** (minimum needed) or **Member**
5. Click **Add**

---

## Step 4: Create Azure Storage Account

This is where the RGB565 image file will be stored for the ESP32 to download.

### 4a: Create the Storage Account

1. Go to **Azure Portal** → **Storage accounts** → **+ Create**
2. Settings:
   - **Subscription**: your subscription
   - **Resource group**: create new, e.g., `rg-pbi-dashboard`
   - **Storage account name**: `pbidashboardstore` (must be globally unique, lowercase, no hyphens)
   - **Region**: choose one close to you
   - **Performance**: Standard
   - **Redundancy**: LRS (cheapest, fine for this)
3. Click **Review + create** → **Create**

### 4b: Create a Blob Container

1. Open the storage account
2. Go to **Containers** (under Data storage)
3. Click **+ Container**
4. Settings:
   - **Name**: `dashboard`
   - **Anonymous access level**: **Blob (anonymous read access for blobs only)**
5. Click **Create**

Note: If "anonymous access level" is greyed out, go to the storage account **Configuration** and set **"Allow Blob anonymous access"** to **Enabled**, then try again.

### 4c: Get the Connection String

1. In the storage account, go to **Access keys**
2. Click **Show** next to key1
3. Copy the **Connection string**

**Save these values:**
```
STORAGE_CONNECTION_STRING = DefaultEndpointsProtocol=https;AccountName=...
STORAGE_CONTAINER_NAME    = dashboard
BLOB_NAME                 = dashboard.rgb565
```

Your ESP32 will eventually fetch from:
```
https://pbidashboardstore.blob.core.windows.net/dashboard/dashboard.rgb565
```
(Replace `pbidashboardstore` with your actual storage account name)

---

## Step 5: Create Azure Function App

### 5a: Create the Function App

1. Go to **Azure Portal** → **Function App** → **+ Create**
2. Settings:
   - **Subscription**: your subscription
   - **Resource group**: `rg-pbi-dashboard` (same as storage)
   - **Function App name**: `pbi-dashboard-func` (must be globally unique)
   - **Runtime stack**: **Python**
   - **Version**: **3.11**
   - **Region**: same as storage account
   - **Operating system**: Linux
   - **Plan type**: **Consumption (Serverless)** — essentially free for this use case
3. Click **Review + create** → **Create**

### 5b: Configure Application Settings

Once created, go to the Function App → **Configuration** (under Settings) → **Application settings**.

Add these settings (click **+ New application setting** for each):

| Name | Value |
|------|-------|
| `PBI_TENANT_ID` | (from Step 2) |
| `PBI_CLIENT_ID` | (from Step 2) |
| `PBI_CLIENT_SECRET` | (from Step 2) |
| `PBI_GROUP_ID` | (from Step 1) |
| `PBI_REPORT_ID` | (from Step 1) |
| `PBI_PAGE_NAME` | (from Step 1, leave blank to use default page) |
| `STORAGE_CONNECTION_STRING` | (from Step 4) |
| `STORAGE_CONTAINER_NAME` | `dashboard` |
| `BLOB_NAME` | `dashboard.rgb565` |

Click **Save** after adding all settings.

---

## Step 6: Verify Everything Before Deploying Code

At this point you should have:

- [ ] Power BI report published in a Premium workspace
- [ ] Group ID and Report ID noted
- [ ] Azure AD app registration with Report.Read.All (admin consent granted)
- [ ] Tenant ID, Client ID, Client Secret noted
- [ ] Service principal enabled in Power BI tenant settings
- [ ] Service principal added to the workspace as Viewer or Member
- [ ] Storage account with public-read blob container
- [ ] Storage connection string noted
- [ ] Function App created with all application settings configured

Once you've confirmed all the above, we'll deploy the Python function code (Phase 5).

---

## Cost Estimate

- **Storage**: ~$0.01/month (one 460KB file rewritten periodically)
- **Function App**: ~$0.00/month on Consumption plan (well within free grant of 1M executions/month)
- **Total**: essentially free
