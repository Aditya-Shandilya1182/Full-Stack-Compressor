from fastAPI import FastAPI
from fastAPI import File, UploadFile
from fastapi.responses import FileResponse
import httpx
import os

app = FastAPI()
storage_dir = "storage"

if not os.path.exists(storage_dir):
    os.makedirs(storage_dir)

cpp_server_url = "http://localhost:8081/api"

@app.post("/compress/")
async def compress(file: UploadFile = File(...)):
    
    file_path = os.path.join(storage_dir, file.filename)
    
    async with aiofiles.open(file_path, 'wb') as out_file:
        content = await file.read()
        await out_file.write(content)

    async with httpx.AsyncClient() as client:
        files = {'file': open(file_path, 'rb')}
        response = await client.post(f"{cpp_server_url}/compress", files=files)

    compressed_file_path = os.path.join(storage_dir, f"compressed_{file.filename}.zst")
    
    with open(compressed_file_path, 'wb') as f:
        f.write(response.content)

    return FileResponse(compressed_file_path, media_type='application/octet-stream', filename=f"compressed_{file.filename}.zst")

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
    
