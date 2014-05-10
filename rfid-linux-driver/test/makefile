# Comment/uncomment the following line to disable/enable debugging 
#DEBUG = y 
DEST_BIN_DIR= drivers/ 
EXTRA_CFLAGS += -D_V3 
#TESTFLAGS = -D_V3 
# Add your debugging flag (or not) to CFLAGS 
ifeq ($(DEBUG),y) 
  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines 
else 
  DEBFLAGS = -O2 
endif 
EXTRA_CFLAGS += $(DEBFLAGS) 
EXTRA_CFLAGS += -I$(LDDINC) 
EXTRA_CFLAGS += -DREV_VERSION=$(REV_VERSION) 
LDFLAGS += --static 
all: test 
clean: 
	rm -rf test_rc522        
cp:
	cp -f test_rc522 $(DEST_BIN_DIR) 
mv:
	mv -f test_rc522 $(DEST_BIN_DIR) 
test: 
	arm-linux-gcc $(CFLAGS) $(LDFLAGS) -O2 test_rc522.c -o test_rc522 
depend .depend dep: 
	$(CC) $(CFLAGS) -M *.c > .depend 


