# client
build guide - the remote ip address should be settup at 'remote.config' file

```bash
$ apt install git cmake gcc g++ libssl-dev libgtkmm-3.0-dev libopencv-dev
$ git clone https://github.com/prayam/cmu_project_public.git
$ cd cmu_project_public/source/client/ && mkdir build; cd build && cmake .. && make
$ vi ./remote.config # modify file to set remote ip address
$ ./client
```
