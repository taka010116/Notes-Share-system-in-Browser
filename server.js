const express = require('express');
const fs = require('fs');
const path = require('path');

const app = express();

app.use(express.static(__dirname));

// 再帰的に.jsonを探す
function findJsonFiles(dir) {
  let results = [];

  const files = fs.readdirSync(dir);

  files.forEach(file => {
    const fullPath = path.join(dir, file);
    const stat = fs.statSync(fullPath);

    if (stat.isDirectory()) {
      results = results.concat(findJsonFiles(fullPath)); // 再帰
    } else if (file.endsWith('.json')) {
      results.push(fullPath);
    }
  });

  return results;
}

app.get('/songs', (req, res) => {
  const songsDir = path.join(__dirname, 'songs');

  let result = [];

  const jsonFiles = findJsonFiles(songsDir);

  jsonFiles.forEach(filePath => {
    try {
      const json = JSON.parse(fs.readFileSync(filePath, 'utf-8'));
      result.push(json);
    } catch (e) {
      console.log('JSON読み込み失敗:', filePath);
    }
  });

  res.json(result);
});

app.listen(3000, () => {
  console.log('http://localhost:3000');
});