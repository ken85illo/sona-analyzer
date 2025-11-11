const textarea_elem = document.getElementById("textarea-element");

const PORT = 8081;
const URL = `http://localhost:${PORT}/api/lexical-analyzer`;

const lexicalAnalyzer = async (text_JSON) => {
    try {
        const rawResponse = await fetch(URL, {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(text_JSON)
        });

        if (!rawResponse.ok) throw new Error("Server error");

        const lexicalAnalysis = await rawResponse.json();
        console.log(lexicalAnalysis);
    } catch (err) {
        console.error("Fetch error:", err);
    }
};

const handleSubmit = () => {

    // Provide format of JSON to be sent
    const text_JSON = {
        text: textarea_elem.value
    }

    console.log("Handle Submit");

    lexicalAnalyzer(text_JSON);
}
