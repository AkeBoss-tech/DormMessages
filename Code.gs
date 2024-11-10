function doPost(e) {
  const id = "1PYgzbwBt9j_kse1N-3AFjUgeklVPQncKSIT2i7-XV8s";
  const sheet = SpreadsheetApp.openById(id).getSheetByName('messages'); // Replace with your sheet name
  const data = JSON.parse(e.postData.contents);
  
  // Add the new entry
  const timestamp = new Date();
  sheet.appendRow([timestamp, data.message]);
  
  // Clean up old messages
  const yesterday = new Date();
  yesterday.setDate(yesterday - 7);
  const lastRow = sheet.getLastRow();
  
  for (let i = lastRow; i >= 2; i--) { // Start from 2 to skip the header row
    const rowTimestamp = new Date(sheet.getRange(i, 1).getValue());
    if (rowTimestamp < yesterday) {
      sheet.deleteRow(i);
    }
  }
  
  return ContentService.createTextOutput(JSON.stringify({ status: "success" }))
      .setMimeType(ContentService.MimeType.JSON);
}

function doGet() {
  const id = "1PYgzbwBt9j_kse1N-3AFjUgeklVPQncKSIT2i7-XV8s"; // Replace with your Google Sheet ID
  const sheet = SpreadsheetApp.openById(id).getSheetByName('messages'); // Replace with your sheet name
  const data = sheet.getDataRange().getValues();
  const messages = [];
  
  for (let i = 1; i < data.length; i++) { // Start from 1 to skip the header row
    messages.push({
      timestamp: data[i][0],
      message: data[i][1]
    });
  }

  console.log(messages);

  return ContentService.createTextOutput(JSON.stringify(messages))
      .setMimeType(ContentService.MimeType.JSON);
}