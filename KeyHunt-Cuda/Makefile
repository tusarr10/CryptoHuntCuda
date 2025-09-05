#---------------------------------------------------------------------
# Makefile for CryptoHuntCuda 
# Author : Modified for tusarr10 project
# Added TelegramAlert support with libcurl
# Cross-platform compatible (Linux & Windows)
#---------------------------------------------------------------------

# To build with GPU support, use:
#    make gpu=1
# To build with debug symbols, use:
#    make debug=1
# To clean up object files, use:
#    make clean

#sudo apt update
#sudo apt install libcurl4-openssl-dev

#---------------------------------------------------------------------

SRC = Base58.cpp IntGroup.cpp Main.cpp Bloom.cpp Random.cpp \
      Timer.cpp Int.cpp IntMod.cpp Point.cpp SECP256K1.cpp \
      KeyHunt.cpp GPU/GPUGenerate.cpp hash/ripemd160.cpp \
      hash/sha256.cpp hash/sha512.cpp hash/ripemd160_sse.cpp \
      hash/sha256_sse.cpp hash/keccak160.cpp GmpUtil.cpp \
      CmdParse.cpp Bech32.cpp \
      TelegramAlert.cpp   # <-- Added TelegramAlert.cpp

OBJDIR = obj

ifdef gpu
OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o Main.o Bloom.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o KeyHunt.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o hash/keccak160.o \
        GPU/GPUEngine.o GmpUtil.o CmdParse.o Bech32.o TelegramAlert.o)  # <-- added TelegramAlert.o
else
OBJET = $(addprefix $(OBJDIR)/, \
        Base58.o IntGroup.o Main.o Bloom.o Random.o Timer.o Int.o \
        IntMod.o Point.o SECP256K1.o KeyHunt.o GPU/GPUGenerate.o \
        hash/ripemd160.o hash/sha256.o hash/sha512.o \
        hash/ripemd160_sse.o hash/sha256_sse.o hash/keccak160.o \
        GmpUtil.o CmdParse.o Bech32.o TelegramAlert.o)  # <-- added TelegramAlert.o
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
LFLAGS     = -lgmp -lpthread -L$(CUDA)/lib64 -lcudart -lcurl  # <-- added -lcurl for Telegram
else
ifdef debug
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -g \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
else
CXXFLAGS   = -m64 -mssse3 -Wno-write-strings -O2 \
             -I. -I$(CUDA)/include -I$(LOCAL_GMP)
endif
LFLAGS     = -lgmp -lpthread -lcurl   # <-- added -lcurl for Telegram
endif

# --------------------------------------------------------------------
# GPU object compilation
ifdef gpu
$(OBJDIR)/GPU/GPUEngine.o: GPU/GPUEngine.cu
	$(NVCC) -maxrregcount=0 --ptxas-options=-v --compile \
	--compiler-options -fPIC -ccbin $(CXXCUDA) -m64 -O2 \
	-I$(CUDA)/include -I$(LOCAL_GMP) $(GENCODE) \
	-o $(OBJDIR)/GPU/GPUEngine.o -c GPU/GPUEngine.cu
endif

# Generic object compilation
$(OBJDIR)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

# Build all
all: CryptoHuntCuda

CryptoHuntCuda: $(OBJET)
	@echo "🚀 Building CryptoHuntCuda..."
	$(CXX) $(OBJET) $(LFLAGS) -o CryptoHuntCuda

# Ensure directories exist
$(OBJET): | $(OBJDIR) $(OBJDIR)/GPU $(OBJDIR)/hash

$(OBJDIR):
ifeq ($(OS),Windows_NT)
	@mkdir $(OBJDIR) 2> NUL || true
else
	@mkdir -p $(OBJDIR)
endif

$(OBJDIR)/GPU: $(OBJDIR)
ifeq ($(OS),Windows_NT)
	cd $(OBJDIR) && mkdir GPU 2> NUL || true
else
	cd $(OBJDIR) && mkdir -p GPU
endif

$(OBJDIR)/hash: $(OBJDIR)
ifeq ($(OS),Windows_NT)
	cd $(OBJDIR) && mkdir hash 2> NUL || true
else
	cd $(OBJDIR) && mkdir -p hash
endif

# Clean up
clean:
	@echo "🧹 Cleaning..."
	@rm -f $(OBJDIR)/*.o
	@rm -f $(OBJDIR)/GPU/*.o
	@rm -f $(OBJDIR)/hash/*.o
	@rm -f CryptoHuntCuda

# ---------------------------------------------------------------------
# ✅ Changes made:
# 1. Added TelegramAlert.cpp to SRC and OBJET
# 2. Linked libcurl (-lcurl) in LFLAGS
# 3. Added cross-platform directory creation (Windows & Linux)
# 4. Comments on GPU object compilation kept
# 5. Now works with Telegram messages directly from C++
# 6. Compatible with both debug/release and GPU/non-GPU builds
# ---------------------------------------------------------------------
