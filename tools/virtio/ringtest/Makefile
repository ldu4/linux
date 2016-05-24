all:

<<<<<<< HEAD
all: ring virtio_ring_0_9 virtio_ring_poll virtio_ring_inorder
=======
all: ring virtio_ring_0_9 virtio_ring_poll virtio_ring_inorder skb_array
>>>>>>> linux-next/akpm-base

CFLAGS += -Wall
CFLAGS += -pthread -O2 -ggdb
LDFLAGS += -pthread -O2 -ggdb

main.o: main.c main.h
ring.o: ring.c main.h
skb_array.o: skb_array.c main.h ../../../include/linux/skb_array.h
virtio_ring_0_9.o: virtio_ring_0_9.c main.h
virtio_ring_poll.o: virtio_ring_poll.c virtio_ring_0_9.c main.h
virtio_ring_inorder.o: virtio_ring_inorder.c virtio_ring_0_9.c main.h
ring: ring.o main.o
virtio_ring_0_9: virtio_ring_0_9.o main.o
virtio_ring_poll: virtio_ring_poll.o main.o
virtio_ring_inorder: virtio_ring_inorder.o main.o
<<<<<<< HEAD
=======
skb_array: skb_array.o main.o
>>>>>>> linux-next/akpm-base
clean:
	-rm main.o
	-rm ring.o ring
	-rm virtio_ring_0_9.o virtio_ring_0_9
	-rm virtio_ring_poll.o virtio_ring_poll
	-rm virtio_ring_inorder.o virtio_ring_inorder

.PHONY: all clean
