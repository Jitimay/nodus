from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import os
from google import genai
import logging
import uvicorn
from dotenv import load_dotenv

load_dotenv()

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s"
)

app = FastAPI(title="Gemini SMS Bridge")

GEMINI_API_KEY = os.getenv("GEMINI_API_KEY")
client = genai.Client(api_key=GEMINI_API_KEY) if GEMINI_API_KEY else None

class SMSRequest(BaseModel):
    sender: str
    message: str

@app.post("/process")
async def process_sms(request: SMSRequest):
    try:
        if not client:
            raise HTTPException(status_code=500, detail="GEMINI_API_KEY is not set. Add it in Andasy dashboard.")

        system_prompt = (
            "You are a helpful and friendly AI assistant. "
            "Keep your answers clear, concise, and under 1000 characters if possible. "
            "Respond in the same language as the user's message (Kirundi, French, English, Swahili, etc.). "
            "Do not use markdown or special formatting."
        )

        response = client.models.generate_content(
            model="gemini-2.5-flash",
            contents=[system_prompt, request.message]
        )

        reply = response.text.strip()
        logging.info(f"From {request.sender}: '{request.message}' -> '{reply[:150]}...'")

        return {"reply": reply}

    except HTTPException:
        raise
    except Exception as e:
        logging.error(f"Error processing request from {request.sender}: {str(e)}")
        raise HTTPException(status_code=500, detail="AI processing failed. Please try again.")

@app.get("/")
async def health_check():
    return {
        "status": "online",
        "message": "Gemini-SMS Bridge is running on Andasy.io"
    }

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
