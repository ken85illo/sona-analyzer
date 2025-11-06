const textarea_elem = document.getElementById("textarea-element");

const handleSubmit = () => {

    // Provide format of JSON to be sent
    const textarea_JSON = {
        text: textarea_elem.textContent
    }

    console.log("Handle Submit");

    const PORT = 8081;
    const URL = `http://localhost:${PORT}/api/lexical-analyzer`;

    fetch(URL, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ textarea_JSON })
        })
        .then(res => res.json())
        .then(data => {
          document.getElementById("textarea-element").textContent = data.text;
          console.log("Fetch")
        });
    
    // Handle DTO mapping of received JSON of tokens and lexemes
}