#!/bin/bash

controllerModule="controllerModule"
modules=(
	"basicWiresModule"
	"exampleModule"
	"memoryModule"
	"simonSaysModule"
	"switchesModule"
)

controllerPort="/dev/cu.usbmodem14211"
ports=(
	"/dev/cu.usbmodem14221"		# basicWiresModule
	""							# exampleModule
	"/dev/cu.usbmodem14241"		# memoryModule
	"/dev/cu.usbmodem14251"		# simonSaysModule
	""							# switchesModule
)

ADDRESS=2

case "$1" in
	"")
		for ((i=0;i<${#modules[@]};++i)); do
			make -C "${modules[i]}" ADDRESS=$ADDRESS
			ADDRESS=$((ADDRESS + 1))
		done 
		make -C "$controllerModule"
		;;

	"clean")
		for ((i=0;i<${#modules[@]};++i)); do
			make -C "${modules[i]}" clean
			ADDRESS=$((ADDRESS + 1))
		done
		make -C "$controllerModule" clean
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
esac
