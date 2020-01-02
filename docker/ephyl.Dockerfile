FROM m1mbert/cxlb-gnuradio-3.7:1.0

RUN ${APT} update && ${APT} dist-upgrade
RUN ${APT} install unzip
RUN ${APT} install wget
RUN ${APT} install libncurses5
RUN ${APT} install libxmu-dev

RUN cd ~/cxlb_toolchain_build

RUN wget https://fr.mathworks.com/supportfiles/downloads/R2014a/deployment_files/R2014a/installers/glnxa64/MCR_R2014a_glnxa64_installer.zip
RUN unzip MCR_R2014a_glnxa64_installer.zip -d MCR_build
RUN rm MCR_R2014a_glnxa64_installer.zip
RUN cd MCR_build
RUN ./install -mode silent -agreeToLicense yes  ## (add sudo for non docker users)
RUN mv /usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/bin/glnxa64/libcurl.so.4 /usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/bin/glnxa64/libcurl.so.4_mcr
RUN mv /usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/sys/os/glnxa64/libstdc++.so.6 /usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/sys/os/glnxa64/libstdc++.so.6_mcr

RUN echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/runtime/glnxa64:/usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/bin/glnxa64:/usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/sys/os/glnxa64' >> ~/.bashrc 
RUN echo 'export XAPPLRESDIR=$XAPPLRESDIR:/usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/X11/app-defaults' >> ~/.bashrc 
RUN echo 'export INSTALL_PATH=/cortexlab/toolchains/current' >> ~/.bashrc
RUN echo 'export MCR_PATH=/usr/local/MATLAB/MATLAB_Compiler_Runtime/v83/extern/include' >> ~/.bashrc
RUN echo 'export CPATH=$CPATH:$MCR_PATH' >> ~/.bashrc

RUN cd ..
RUN git clone git://github.com/CorteXlab/gr-ephyl.git
RUN cd gr-ephyl

## THIS DOESNT WORK (??)
# cp include/libTurboFSK_v4.h /cortexlab/toolchains/current/include/
#cp lib/libTurboFSK_v4.so /cortexlab/toolchains/current/lib/

## BUT THIS WORKS (??)
RUN cp include/libTurboFSK_v4.h /usr/include/
RUN cp lib/libTurboFSK_v4.so /usr/lib/
RUN mkdir build
RUN cd build
RUN cmake -DCMAKE_INSTALL_PREFIX=${INSTALL_PATH} ..
RUN make
RUN make install
RUN cp ../apps/hier_* ~/.grc_gnuradio/
RUN cd ~
