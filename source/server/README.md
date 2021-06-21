#### Build the project
```bash
git clone https://github.com/prayam/cmu_project_public.git
cd cmu_project/source/server
python3 step01_pb_to_uff.py
rm -rf MTCNN_FaceDetection_TensorRT/
git clone https://github.com/PKUZHOU/MTCNN_FaceDetection_TensorRT
mv MTCNN_FaceDetection_TensorRT/det* ./mtCNNModels
mkdir build; cd build
cmake ..
make
sudo systemctl restart nvargus-daemon && ./LgFaceRecDemoTCP_Jetson_NanoV2 5000
```
