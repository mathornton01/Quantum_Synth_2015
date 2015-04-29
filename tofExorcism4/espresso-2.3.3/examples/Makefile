BINARY = ../espresso
TARGETS = dc1.out mlp4.out Z5xp1.out bca.out cps.out opa.out ti.out tial.out

all: $(TARGETS)

dc1.out: dc1 $(BINARY)
	time $(BINARY) -t dc1 >dc1.out
mlp4.out: mlp4 $(BINARY)
	time $(BINARY) -t mlp4 >mlp4.out
Z5xp1.out: Z5xp1 $(BINARY)
	time $(BINARY) -t Z5xp1 >Z5xp1.out
bca.out: bca $(BINARY)
	time $(BINARY) -t bca >bca.out
cps.out: cps $(BINARY)
	time $(BINARY) -t cps >cps.out
opa.out: opa $(BINARY)
	time $(BINARY) -t opa >opa.out
ti.out: ti $(BINARY)
	time $(BINARY) -t ti >ti.out
tial.out: tial $(BINARY)
	time $(BINARY) -t tial >tial.out

clean:
	rm -f $(TARGETS) Make.out
