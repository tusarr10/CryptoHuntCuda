#---------------------------------------------------------------------
# Makefile for CryptoHuntCuda 
#
# Author :  Modified for tusarr10 project
#---------------------------------------------------------------------

SRC = Base58.cpp IntGroup.cpp Main.cpp Bloom.cpp Random.cpp \
      Timer.cpp Int.cpp IntMod.cpp Point.cpp SECP256K1.cpp \
      KeyHunt.cpp GPU/GPUGenerate.cpp hash/ripemd160.cpp \
      hash/sha256.cpp hash/sha512.cpp hash/ripemd160_sse.cpp \
      hash/sha256_sse.cpp hash/keccak160.cpp GmpUtil.cpp \
      CmdParse.cpp Bech32.cpp

OBJDIR = obj

ifdef gpu
OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o Main.o Bloom.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o KeyHunt.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o hash/keccak160.o \
        GPU/GPUEngine.o GmpUtil.o CmdParse.o Bech32.o)
else
OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o Main.o Bloom.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o KeyHunt.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o hash/keccak160.o \
        GmpUtil.o CmdParse.o Bech32.o)
endif

CXX        = g++
CUDA       = /usr/local/cuda-11.4
CXXCUDA    = /usr/bin/g++
NVCC       = $(CUDA)/bin/nvcc

# --------------------------------------------------------------------
# Local GMP include path (inside repo)
LOCAL_GMP  = gmp/include

# Universal GPU architecture support (Kepler → Ada + PTX fallback)
GENCODE   = \
  -gencode=arch=compute_35,code=sm_35 \
  -gencode=arch=compute_52,code=sm_52 \
  -gencode=arch=compute_61,code=sm_61 \
  -gencode=arch=compute_75,code=sm_75 \
  -gencode=arch=compute_86,code=sm_86


# --------------------------------------------------------------------

ifdef gpu
ifdef debug
CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -g \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
else
CXXFLAGS   = -DWITHGPU -m64 -mssse3 -Wno-write-strings -O2 \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
endif
LFLAGS     = -lgmp -lpthread -L$(CUDA)/lib64 -lcudart
else
ifdef debug
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -g \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
else
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -O2 \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
endif
LFLAGS     = -lgmp -lpthread
endif

# --------------------------------------------------------------------

ifdef gpu
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -maxrregcount=0 --ptxas-options=-v --compile \
	--compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -O2 \
	-I$(CUDA)/include -I$(LOCAL_GMP) $(GENCODE) \
	-o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
endif

$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

all: CryptoHuntCuda

CryptoHuntCuda: $(OBJET)
	@echo "🚀 Building CryptoHuntCuda..."
	$(CXX) $(OBJET) $(LFLAGS) -o CryptoHuntCuda

$(OBJET): | $(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/GPU: $(OBJDIR)
	cd $(OBJDIR) && mkdir -p GPU

$(OBJDIR)/hash: $(OBJDIR)
	cd $(OBJDIR) && mkdir -p hash

clean:
	@echo "🧹 Cleaning..."
	@rm -f obj/*.o
	@rm -f obj/GPU/*.o
	@rm -f obj/hash/*.o
	@rm -f CryptoHuntCuda
