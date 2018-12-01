#!/bin/bash

controllerModule="controllerModule"
configModule="configModule"
modules=(
	"basicWiresModule"
	"exampleModule"
	"memoryModule"
	"simonSaysModule"
	"switchesModule"
	"morseCodeModule"
)

controllerPort="/dev/cu.usbmodem14211"
configPort="/dev/cu.usbmodem14211"
ports=(
	"/dev/cu.usbmodemUSB0"		# basicWiresModule
	""							# exampleModule
	"/dev/cu.usbmodemUSB1"		# memoryModule
	"/dev/cu.usbmodemUSB2"		# simonSaysModule
	"/dev/cu.usbmodemUSB3"		# switchesModule
	"/dev/cu.usbmodemUSB4"		# morseCodeModule
)

ADDRESS=5

case "$1" in
	"")
		for ((i=0;i<${#modules[@]};++i)); do
			make -C "${modules[i]}" ADDRESS=$ADDRESS
			ADDRESS=$((ADDRESS + 1))
		done 
		make -C "$controllerModule"
		make -C "$configModule"
		;;

	"clean")
		for ((i=0;i<${#modules[@]};++i)); do
			make -C "${modules[i]}" clean
			ADDRESS=$((ADDRESS + 1))
		done
		make -C "$controllerModule" clean
		make -C "$configModule" clean
		;;

	"upload")
		for ((i=0;i<${#modules[@]};++i)); do
			if [ "${ports[i]}" != "" ]; then
				make -C "${modules[i]}" ADDRESS=$ADDRESS MONITOR_PORT="${ports[i]}" upload
				ADDRESS=$((ADDRESS + 1))
				printf "%s (%d) => %s\n\n" "${modules[i]}" "$ADDRESS" "${ports[i]}"
			fi
		done
		make -C "$controllerModule" MONITOR_PORT="$controllerPort" upload
		printf "%s => %s\n\n" "$controllerModule" "$controllerPort"
		;;

	"handbook")
		qpdf --empty --pages */instructions.pdf -- handbook.pdf
		;;
esac
