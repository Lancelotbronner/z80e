//
//  Utilities.swift
//  Tests/z80e
//
//  Created by Christophe Bronner on 2024-03-08.
//

import XCTest
import z80e

open class XCTestCaseAsic: XCTestCase {

	//MARK: - Device Management

	open var _type: ti_device_type { TI83p }

	public internal(set) var _device: UnsafeMutablePointer<asic_t>!

	public var device: asic_t {
		_read { yield _device.pointee }
		_modify { yield &_device.pointee }
	}

	public func flash(_ data: [UInt8]) {
		for i in data.indices {
			device.mmu.flash[i] = data[i];
		}
	}

	public func read_byte(_ address: UInt16) -> UInt8 {
		cpu_read_byte(&device.cpu, address)
	}

	public func write_byte(_ address: UInt16, _ value: UInt8) {
		cpu_write_byte(&device.cpu, address, value)
	}

	public func read_word(_ address: UInt16) -> UInt16 {
		cpu_read_word(&device.cpu, address)
	}

	public func write_word(_ address: UInt16, _ value: UInt16) {
		cpu_write_word(&device.cpu, address, value)
	}

	//MARK: - Test Device

	public var _value: UInt8 = 0

	//MARK: - Hooks

	override public func setUp() {
		guard let device = asic_init(_type, nil) else {
			XCTFail("Failed to initialize device")
			return
		}
		_device = device

		// Configure the default test device
		cpu_device(&self.device.cpu, 0x12)[0] = z80_device(
			data: Unmanaged<XCTestCaseAsic>
				.passUnretained(self)
				.toOpaque(),
			read: test_read,
			write: test_write)
	}

	override public func tearDown() {
		asic_free(_device)
	}

}

private func test_read(_ device: UnsafeMutableRawPointer!) -> UInt8 {
	let test = Unmanaged<XCTestCaseAsic>
		.fromOpaque(device)
		.takeUnretainedValue()
	return test._value
}

private func test_write(_ device: UnsafeMutableRawPointer!, _ value: UInt8) {
	let test = Unmanaged<XCTestCaseAsic>
		.fromOpaque(device)
		.takeUnretainedValue()
	test._value = value
}