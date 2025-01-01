#!/bin/bash

urls=(
    # gstreamer_depends_urls
    "http://launchpadlibrarian.net/601697838/libpcre3-dev_8.39-13ubuntu0.22.04.1_arm64.deb"
    "http://launchpadlibrarian.net/601697841/libpcre3_8.39-13ubuntu0.22.04.1_arm64.deb"
    "http://launchpadlibrarian.net/728293808/libc6_2.35-0ubuntu3.8_arm64.deb"
    "http://launchpadlibrarian.net/581262416/libcrypt1_4.4.27-1.1_arm64.deb"
    "http://launchpadlibrarian.net/665999506/libgcc-s1_12.3.0-1ubuntu1~22.04_arm64.deb"
    "http://launchpadlibrarian.net/665999430/gcc-12-base_12.3.0-1ubuntu1~22.04_arm64.deb"
    "http://launchpadlibrarian.net/580783518/libffi-dev_3.4.2-4_arm64.deb"

    # gstreamer_video_urls
    "http://launchpadlibrarian.net/581134060/liborc-0.4-dev_0.4.32-2_arm64.deb"

    # opencv_urls
    "http://launchpadlibrarian.net/655171230/libopencv-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171249/python3-opencv_4.6.0+dfsg-11_arm64.deb"

    # pygobject_urls
    "http://launchpadlibrarian.net/591097115/python-gi-dev_3.42.0-3build1_arm64.deb"

    # libzmq_urls
    "http://launchpadlibrarian.net/574365959/libzmq3-dev_4.3.4-2_arm64.deb"
    "http://launchpadlibrarian.net/739867057/libgssapi-krb5-2_1.19.2-2ubuntu0.4_arm64.deb"
    "http://launchpadlibrarian.net/739867051/krb5-multidev_1.19.2-2ubuntu0.4_arm64.deb"
    "http://launchpadlibrarian.net/592808726/libsodium-dev_1.0.18-1build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libs/libsodium/libsodium23_1.0.18-1build2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libp/libpgm/libpgm-dev_5.3.128~dfsg-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/n/norm/libnorm-dev_1.5.9+dfsg-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/n/norm/libnorm1_1.5.9+dfsg-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libb/libbsd/libbsd-dev_0.11.5-1_arm64.deb"

    # norm_urls
    "http://ports.ubuntu.com/pool/main/libx/libxml2/libxml2-dev_2.9.13+dfsg-1build1_arm64.deb"

    # gio_urls
    "http://ports.ubuntu.com/pool/main/z/zlib/zlib1g-dev_1.2.11.dfsg-2ubuntu9_arm64.deb"
    "http://ports.ubuntu.com/pool/main/u/util-linux/libmount-dev_2.37.2-4ubuntu3_arm64.deb"

    # mount_urls
    "http://ports.ubuntu.com/pool/main/u/util-linux/libblkid-dev_2.37.2-4ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libs/libselinux/libselinux1-dev_3.3-1build2_arm64.deb"

    # libselinux_urls
    "http://ports.ubuntu.com/pool/main/libs/libsepol/libsepol-dev_3.3-1build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/pcre2/libpcre2-dev_10.39-3build1_arm64.deb"

    # open_cv_dev_urls
    "http://launchpadlibrarian.net/655171185/libopencv-core-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171229/libopencv-core406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171194/libopencv-highgui-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171234/libopencv-highgui406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171180/libopencv-calib3d-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171227/libopencv-calib3d406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171183/libopencv-contrib-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171228/libopencv-contrib406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171188/libopencv-dnn-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171231/libopencv-dnn406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171190/libopencv-features2d-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171232/libopencv-features2d406_4.6.0+dfsg-11_arm64.deb"
    
    "http://launchpadlibrarian.net/655171192/libopencv-flann-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171233/libopencv-flann406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171196/libopencv-imgcodecs-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171235/libopencv-imgcodecs406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171198/libopencv-imgproc-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171236/libopencv-imgproc406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171201/libopencv-ml-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171237/libopencv-ml406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171203/libopencv-objdetect-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171238/libopencv-objdetect406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171206/libopencv-photo-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171239/libopencv-photo406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171210/libopencv-shape-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171240/libopencv-shape406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171212/libopencv-stitching-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171241/libopencv-stitching406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171215/libopencv-superres-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171242/libopencv-superres406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171244/libopencv-video406_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171217/libopencv-video-dev_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171245/libopencv-videoio406_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171219/libopencv-videoio-dev_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171221/libopencv-videostab-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171246/libopencv-videostab406_4.6.0+dfsg-11_arm64.deb"

    "http://launchpadlibrarian.net/655171223/libopencv-viz-dev_4.6.0+dfsg-11_arm64.deb"
    "http://launchpadlibrarian.net/655171247/libopencv-viz406_4.6.0+dfsg-11_arm64.deb"

    "http://ports.ubuntu.com/pool/universe/z/zeromq3/libzmq5_4.3.4-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/t/tbb/libtbb2_2020.3-1ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/z/zlib/zlib1g_1.2.11.dfsg-2ubuntu9_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/o/openexr/libopenexr25_2.5.7-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/g/gdal/libgdal30_3.4.1+dfsg-1build4_arm64.deb"
    "http://ports.ubuntu.com/pool/main/o/openjpeg2/libopenjp2-7_2.4.0-6_arm64.deb"
    "http://ports.ubuntu.com/pool/main/t/tiff/libtiff5_4.3.0-6_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/g/gdcm/libgdcm3.0_3.0.10-1build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libp/libpng1.6/libpng16-16_1.6.37-3build5_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libw/libwebp/libwebp7_1.2.2-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libj/libjpeg8-empty/libjpeg8_8c-2ubuntu10_arm64.deb"
    "http://ports.ubuntu.com/pool/main/o/openssl/libssl3_3.0.2-0ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/i/ilmbase/libilmbase25_2.5.7-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libd/libdeflate/libdeflate0_1.10-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/j/jbigkit/libjbig0_2.1-3.1build3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/x/xz-utils/liblzma5_5.2.5-2ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libz/libzstd/libzstd1_1.4.8+dfsg-3build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/e/expat/libexpat1_2.4.7-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/j/json-c/libjson-c5_0.15-2build4_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/c/charls/libcharls2_2.3.4-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/u/util-linux/libuuid1_2.37.2-4ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/s/sqlite3/libsqlite3-0_3.37.2-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/s/spatialite/libspatialite7_5.0.1-2build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/pcre2/libpcre2-8-0_10.39-3build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/c/curl/libcurl4_7.81.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/f/fyba/libfyba0_4.1.1-7_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libx/libxml2/libxml2_2.9.13+dfsg-1build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/m/mysql-8.0/libmysqlclient21_8.0.28-0ubuntu4_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/o/ogdi-dfsg/libogdi4.1_4.1.0+ds-5_arm64.deb"
    "http://ports.ubuntu.com/pool/main/g/giflib/libgif7_5.1.9-2build2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libg/libgeotiff/libgeotiff5_1.7.0-2build1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/c/cfitsio/libcfitsio9_4.0.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/postgresql-14/libpq5_14.2-1ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/l/lz4/liblz4-1_1.9.3-2build2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/c/c-blosc/libblosc1_1.21.1+ds2-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/p/proj/libproj22_8.2.1-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libh/libhdf4/libhdf4-0-alt_4.2.15-4_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libh/libheif/libheif1_1.12.0-2build1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/a/armadillo/libarmadillo10_10.8.2+dfsg-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/poppler/libpoppler118_22.02.0-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/f/freexl/libfreexl1_1.0.6-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/q/qhull/libqhull-r8.0_2020.2-4_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/g/geos/libgeos-c1v5_3.10.2-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/u/unixodbc/libodbc2_2.3.9-5_arm64.deb"
    "http://ports.ubuntu.com/pool/main/u/unixodbc/libodbcinst2_2.3.9-5_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libk/libkml/libkmlbase1_1.3.0-9_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libk/libkml/libkmldom1_1.3.0-9_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libk/libkml/libkmlengine1_1.3.0-9_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/x/xerces-c/libxerces-c3.2_3.2.3+debian-3build1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/n/netcdf/libnetcdf19_4.8.1-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/h/hdf5/libhdf5-103-1_1.10.7+repack-4ubuntu2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libj/libjpeg-turbo/libjpeg-turbo8_2.1.2-0ubuntu1_arm64.deb"

    # libgdal, libheif, libpoppler, libgeos_c, libodbc, libkmlbase, libxerces-c-3.2.so, libnetcdf.so.19 ...
    "http://ports.ubuntu.com/pool/universe/a/aom/libaom3_3.3.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libd/libde265/libde265-0_1.0.8-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/x/x265/libx265-199_3.5-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/d/dav1d/libdav1d5_0.9.2-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/f/freetype/libfreetype6_2.11.1+dfsg-1build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/f/fontconfig/libfontconfig1_2.13.1-4.2ubuntu5_arm64.deb"
    "http://ports.ubuntu.com/pool/main/l/lcms2/liblcms2-2_2.12~rc1-2build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/nss/libnss3_3.68.2-0ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/nspr/libnspr4_4.32-3build1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/g/geos/libgeos3.10.2_3.10.2-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libt/libtool/libltdl7_2.4.6-15build2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/m/minizip/libminizip1_1.1-8build1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/u/uriparser/liburiparser1_0.9.6+dfsg-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/c/curl/libcurl3-gnutls_7.81.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/i/icu/libicu70_70.1-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/h/hdf5/libhdf5-103-1_1.10.7+repack-4ubuntu2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/h/hdf5/libhdf5-hl-100_1.10.7+repack-4ubuntu2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/liba/libaec/libsz2_1.0.6-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libt/libtirpc/libtirpc3_1.3.2-2build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/b/bzip2/libbz2-1.0_1.0.8-5build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/o/openldap/libldap-2.5-0_2.5.11+dfsg-1~exp1ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/s/snappy/libsnappy1v5_1.1.8-1build3_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libr/librttopo/librttopo1_1.1.0-2_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/g/geos/libgeos-c1v5_3.10.2-1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libr/librttopo/librttopo1_1.1.0-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/nghttp2/libnghttp2-14_1.43.0-1build3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libi/libidn2/libidn2-0_2.3.2-2build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/r/rtmpdump/librtmp1_2.4+20151223.gitfa8646d.1-2build4_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libs/libssh/libssh-4_0.9.6-2build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libp/libpsl/libpsl5_0.21.0-1.2build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/o/openldap/libldap-2.5-0_2.5.11+dfsg-1~exp1ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/b/brotli/libbrotli1_1.0.9-2build6_arm64.deb"
    "http://ports.ubuntu.com/pool/main/k/krb5/libkrb5-3_1.19.2-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/k/krb5/libk5crypto3_1.19.2-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/e/e2fsprogs/libcom-err2_1.46.5-2ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/k/krb5/libkrb5support0_1.19.2-2_arm64.deb"

    "http://ports.ubuntu.com/pool/main/l/lapack/libblas3_3.10.0-2ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/l/lapack/liblapack3_3.10.0-2ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/a/arpack/libarpack2_3.8.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/g/gcc-12/libgfortran5_12-20220319-1ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/s/superlu/libsuperlu5_5.3.0+dfsg1-2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/numactl/libnuma1_2.0.14-3ubuntu2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/nettle/libnettle8_3.7.3-1build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/g/gnutls28/libgnutls30_3.7.3-4ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/liba/libaec/libaec0_1.0.6-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/c/cyrus-sasl2/libsasl2-2_2.1.27+dfsg2-3ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libu/libunistring/libunistring2_1.0-1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/n/nettle/libhogweed6_3.7.3-1build2_arm64.deb"
    "http://ports.ubuntu.com/pool/main/g/gmp/libgmp10_6.2.1+dfsg-3ubuntu1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/k/keyutils/libkeyutils1_1.6.1-2ubuntu3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/p11-kit/libp11-kit0_0.24.0-6build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libt/libtasn1-6/libtasn1-6_4.18.0-4build1_arm64.deb"
    "http://ports.ubuntu.com/pool/main/libf/libffi/libffi8_3.4.2-4_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/libf/libffi7/libffi7_3.3-5ubuntu1_arm64.deb"

    # extra for new version of opencv
    "http://launchpadlibrarian.net/644079349/libglx0_1.6.0-1_arm64.deb"
    "http://launchpadlibrarian.net/643209021/libtbb12_2021.8.0-1ubuntu2_arm64.deb"
    "http://launchpadlibrarian.net/644079348/libglvnd0_1.6.0-1_arm64.deb"
    "http://launchpadlibrarian.net/690071619/libx11-6_1.8.4-2ubuntu0.3_arm64.deb"
    "http://launchpadlibrarian.net/616901200/libxcb1_1.15-1_arm64.deb"
    "http://launchpadlibrarian.net/592818831/libxau6_1.0.9-1build5_arm64.deb"
    "http://launchpadlibrarian.net/592820467/libxdmcp6_1.1.3-0ubuntu5_arm64.deb"
    "http://launchpadlibrarian.net/582055787/libbsd0_0.11.5-1_arm64.deb"
    "http://launchpadlibrarian.net/592766771/libmd0_1.0.4-1build1_arm64.deb"
    "http://launchpadlibrarian.net/649832745/libgdal32_3.6.2+dfsg-1build1_arm64.deb"
    "http://launchpadlibrarian.net/691213656/libtiff6_4.5.0-5ubuntu1.2_arm64.deb"
    "http://launchpadlibrarian.net/618732220/libopenexr-3-1-30_3.1.5-4_arm64.deb"
    "http://launchpadlibrarian.net/635704580/liblerc4_4.0.0+ds-2ubuntu2_arm64.deb"
    "http://launchpadlibrarian.net/621577890/libimath-3-1-29_3.1.5-1ubuntu2_arm64.deb"
    "http://launchpadlibrarian.net/632281962/libarmadillo11_11.4.2+dfsg-1_arm64.deb"
    "http://launchpadlibrarian.net/680101168/libpoppler126_22.12.0-2ubuntu1.1_arm64.deb"
    "http://launchpadlibrarian.net/641897835/libcfitsio10_4.2.0-3_arm64.deb"
    "http://launchpadlibrarian.net/649681668/libproj25_9.1.1-1build1_arm64.deb"
    "http://launchpadlibrarian.net/649707776/libopenjp2-7_2.5.0-1build1_arm64.deb"
    "http://launchpadlibrarian.net/633923215/libgeos3.11.1_3.11.1-1_arm64.deb"
    "http://launchpadlibrarian.net/633923214/libgeos-c1v5_3.11.1-1_arm64.deb"
)

# Directory to extract packages
extract_dir="/tmp/debian_packages_dir"

# System library directory (adjust as needed)
lib_dir="/usr/lib/aarch64-linux-gnu"

# Create extraction directory
mkdir -p $extract_dir

# Download and extract with retry logic
for url in "${urls[@]}"; do
    # Get the package name from the URL
    package_name=$(basename "$url")

    # Define retry count and success flag
    retry_count=0
    max_retries=5
    success=0

    # Try downloading the package with retries
    while [[ $retry_count -lt $max_retries ]]; do
        echo "Attempting to download $url (Attempt $((retry_count+1))/$max_retries)"

        # Download the package
        wget "$url" -P "$extract_dir/"

        # Check if wget succeeded
        if [[ $? -eq 0 ]]; then
            echo "Download succeeded!"
            success=1
            break
        else
            echo "Download failed. Retrying..."
            retry_count=$((retry_count+1))
        fi
    done

    # If the download failed after max retries, exit with error
    if [[ $success -eq 0 ]]; then
        echo "Failed to download $url after $max_retries attempts. Exiting with error."
        exit 1
    fi

    # Extract the package
    echo "Extracting $package_name"
    dpkg -x "$extract_dir/$package_name" "$extract_dir"
done


# Copy the extracted libraries to the system library directory
sudo cp -r $extract_dir/usr/lib/aarch64-linux-gnu/* $lib_dir/
sudo cp -r $extract_dir/usr/lib/*so* $lib_dir/
sudo cp -r $extract_dir/lib/aarch64-linux-gnu/* $lib_dir/
sudo cp -r /usr/lib/aarch64-linux-gnu/pkgconfig/mit-krb5/* /usr/lib/aarch64-linux-gnu/pkgconfig/
sudo cp -r $extract_dir/usr/lib/aarch64-linux-gnu/blas/* $lib_dir/
sudo cp -r $extract_dir/usr/lib/aarch64-linux-gnu/lapack/* $lib_dir/

python_extract_dir="/tmp/python_debian_packages_dir"

python_urls=(
    # python 3.10
    "http://launchpadlibrarian.net/568513787/libpython3.10-dev_3.10.0-4_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/python3.10/libpython3.10_3.10.4-3_arm64.deb"
    "http://ports.ubuntu.com/pool/main/p/python3.10/python3.10-minimal_3.10.4-3_arm64.deb"
    # python 3.11
    "http://ports.ubuntu.com/pool/universe/p/python3.11/libpython3.11-dev_3.11.0~rc1-1~22.04_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/p/python3.11/libpython3.11_3.11.0~rc1-1~22.04_arm64.deb"
    "http://ports.ubuntu.com/pool/universe/p/python3.11/python3.11-minimal_3.11.0~rc1-1~22.04_arm64.deb"
)

# Download, extract, and copy libraries
for url in "${python_urls[@]}"; do
    # Get the package name from the URL
    package_name=$(basename "$url")

    # Define retry count and success flag
    retry_count=0
    max_retries=5
    success=0

    # Try downloading the package with retries
    while [[ $retry_count -lt $max_retries ]]; do
        echo "Attempting to download $url (Attempt $((retry_count+1))/$max_retries)"

        # Download the package
        wget "$url" -P "$python_extract_dir/"

        # Check if wget succeeded
        if [[ $? -eq 0 ]]; then
            echo "Download succeeded!"
            success=1
            break
        else
            echo "Download failed. Retrying..."
            retry_count=$((retry_count+1))
        fi
    done

    # If the download failed after max retries, exit or skip
    if [[ $success -eq 0 ]]; then
        echo "Failed to download $url after $max_retries attempts. Exiting with error."
        exit 1
    fi

    # Extract the package
    echo "Extracting $package_name"
    dpkg -x "$python_extract_dir/$package_name" "$python_extract_dir"
done


# python 3.10
sudo mkdir /usr/include/aarch64-linux-gnu/
sudo mkdir /usr/include/aarch64-linux-gnu/python3.10/

sudo cp -r $python_extract_dir/usr/include/aarch64-linux-gnu/python3.10/pyconfig.h /usr/include/aarch64-linux-gnu/python3.10/
sudo cp -r $python_extract_dir/usr/lib/* /usr/lib/
sudo cp -r $python_extract_dir/usr/include/python3.10/* /usr/include/python3.10/
sudo cp -r $python_extract_dir/usr/bin/aarch64-linux-gnu-python3.10-config /usr/bin/

# python 3.11
sudo mkdir /usr/include/aarch64-linux-gnu/python3.11/

sudo cp -r $python_extract_dir/usr/include/aarch64-linux-gnu/python3.11/pyconfig.h /usr/include/aarch64-linux-gnu/python3.11/
sudo cp -r $python_extract_dir/usr/lib/* /usr/lib/
sudo cp -r $python_extract_dir/usr/include/python3.11/* /usr/include/python3.11/
sudo cp -r $python_extract_dir/usr/bin/aarch64-linux-gnu-python3.11-config /usr/bin/

# python 3.9
sudo dpkg --add-architecture arm64
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo mkdir /usr/include/aarch64-linux-gnu/python3.9/
py39_packages=(
  "python3.9-minimal:arm64"
  "libpython3.9-dev:arm64"
  "libpython3.9:arm64"
)

# Download and extract each package
for package in "${py39_packages[@]}"; do
    # Download the package
    sudo apt-get download "$package"
done

# Extract the downloaded .deb files
for deb_file in *python3.9*.deb; do
    dpkg -x "$deb_file" "$python_extract_dir"
done

sudo cp -r $python_extract_dir/usr/include/aarch64-linux-gnu/python3.9/pyconfig.h /usr/include/aarch64-linux-gnu/python3.9/
sudo cp -r $python_extract_dir/usr/lib/* /usr/lib/
sudo cp -r $python_extract_dir/usr/include/python3.9/* /usr/include/python3.9/
sudo cp -r $python_extract_dir/usr/bin/aarch64-linux-gnu-python3.9-config /usr/bin/

# python 3.8
sudo mkdir /usr/include/aarch64-linux-gnu/python3.8/
py38_packages=(
  "python3.8-minimal:arm64"
  "libpython3.8-dev:arm64"
  "libpython3.8:arm64"
)

# Download and extract each package
for package in "${py38_packages[@]}"; do
    # Download the package
    sudo apt-get download "$package"
done

# Extract the downloaded .deb files
for deb_file in *python3.8*.deb; do
    dpkg -x "$deb_file" "$python_extract_dir"
done

sudo cp -r $python_extract_dir/usr/include/aarch64-linux-gnu/python3.8/pyconfig.h /usr/include/aarch64-linux-gnu/python3.8/
sudo cp -r $python_extract_dir/usr/lib/* /usr/lib/
sudo cp -r $python_extract_dir/usr/include/python3.8/* /usr/include/python3.8/
sudo cp -r $python_extract_dir/usr/bin/aarch64-linux-gnu-python3.8-config /usr/bin/
echo "Debian packages finished installed."
