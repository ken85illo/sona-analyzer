const express = require('express');
const cors = require('cors');
const app = express();

// Middleware for parsing JSON
app.use(express.json());

// Enable CORS for all routes and origins
app.use(cors());

// POST - lexical analyzer endpoint
app.post('/api/lexical-analyzer', (req, res) => {
  // Text area value - adjust if text area DTO changes
  const text = req.body.text;
  
  // Insert cross communication with CPP executable


  // Response template -- correspond to lexeme and tokens DTO
  const response = {
    text: "This is backend response"
  }
  
  // Return response statement
  res.json(response);
});

// Localhost port
const PORT = 8081;

app.listen(PORT, () => {
  console.log('REST API server running on port 8081');
});