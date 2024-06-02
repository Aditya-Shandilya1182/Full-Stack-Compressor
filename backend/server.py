import os
from fastapi import FastAPI, File, UploadFile
from fastapi.responses import FileResponse
from fastapi.middleware.cors import CORSMiddleware
import httpx
import aiofiles
import base64

app = FastAPI()
storage_dir = "storage"

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["GET", "POST"],
    allow_headers=["*"],
)

if not os.path.exists(storage_dir):
    os.makedirs(storage_dir)

cpp_server_url = "http://localhost:8081"

@app.post("/compress/")
async def compress(file: UploadFile = File(...)):
    file_path = os.path.join(storage_dir, file.filename)
    
    # Save the uploaded file
    async with aiofiles.open(file_path, 'wb') as out_file:
        content = await file.read()
        await out_file.write(content)

    # Send the file to the C++ server for compression
    async with httpx.AsyncClient() as client:
        with open(file_path, 'rb') as f:
            response = await client.post(
                f"{cpp_server_url}/compress",
                content=f.read()
            )
    
    response_data = response.json()
    compressed_data = base64.b64decode(response_data['compressedData'])

    compressed_file_path = os.path.join(storage_dir, f"compressed_{file.filename}.zst")
    
    # Save the compressed file received from the C++ server
    async with aiofiles.open(compressed_file_path, 'wb') as out_file:
        await out_file.write(compressed_data)

    return FileResponse(compressed_file_path, media_type='application/octet-stream', filename=f"compressed_{file.filename}.zst")

@app.post("/decompress/")
async def decompress(file: UploadFile = File(...)):
    file_path = os.path.join(storage_dir, file.filename)
    
    # Save the uploaded file
    async with aiofiles.open(file_path, 'wb') as out_file:
        content = await file.read()
        await out_file.write(content)

    # Read the compressed data
    async with aiofiles.open(file_path, 'rb') as in_file:
        compressed_data = await in_file.read()

    # Send the compressed data to the C++ server for decompression
    async with httpx.AsyncClient() as client:
        response = await client.post(
            f"{cpp_server_url}/decompress",
            json={
                "compressedData": base64.b64encode(compressed_data).decode('utf-8')
            }
        )

    decompressed_file_path = os.path.join(storage_dir, f"decompressed_{file.filename[:-4]}")
    
    # Save the decompressed file received from the C++ server
    async with aiofiles.open(decompressed_file_path, 'wb') as out_file:
        await out_file.write(response.content)

    return FileResponse(decompressed_file_path, media_type='application/octet-stream', filename=f"decompressed_{file.filename[:-4]}")

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
