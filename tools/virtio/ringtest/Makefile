all:

<<<<<<< HEAD
all: ring virtio_ring_0_9 virtio_ring_poll virtio_ring_inorder noring
=======
all: ring virtio_ring_0_9 virtio_ring_poll virtio_ring_inorder ptr_ring noring
>>>>>>> linux-next/akpm-base

CFLAGS += -Wall
CFLAGS += -pthread -O2 -ggdb
LDFLAGS += -pthread -O2 -ggdb

main.o: main.c main.h
ring.o: ring.c main.h
ptr_ring.o: ptr_ring.c main.h ../../../include/linux/ptr_ring.h
virtio_ring_0_9.o: virtio_ring_0_9.c main.h
virtio_ring_poll.o: virtio_ring_poll.c virtio_ring_0_9.c main.h
virtio_ring_inorder.o: virtio_ring_inorder.c virtio_ring_0_9.c main.h
ring: ring.o main.o
virtio_ring_0_9: virtio_ring_0_9.o main.o
virtio_ring_poll: virtio_ring_poll.o main.o
virtio_ring_inorder: virtio_ring_inorder.o main.o
<<<<<<< HEAD
=======
ptr_ring: ptr_ring.o main.o
>>>>>>> linux-next/akpm-base
noring: noring.o main.o
clean:
	-rm main.o
	-rm ring.o ring
	-rm virtio_ring_0_9.o virtio_ring_0_9
	-rm virtio_ring_poll.o virtio_ring_poll
	-rm virtio_ring_inorder.o virtio_ring_inorder
<<<<<<< HEAD
=======
	-rm ptr_ring.o ptr_ring
>>>>>>> linux-next/akpm-base
	-rm noring.o noring

.PHONY: all clean
