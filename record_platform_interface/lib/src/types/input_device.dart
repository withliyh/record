class InputDevice {
  /// The ID used to select the device on the platform.
  final String id;

  /// The label text representation.
  final String label;

  const InputDevice({
    required this.id,
    required this.label,
  });

  factory InputDevice.fromMap(Map map) => InputDevice(
        id: map['id'],
        label: map['label'],
      );

  Map<String, dynamic> toMap() => {
        'id': id,
        'label': label,
      };

  @override
  String toString() {
    return '''
      id: $id
      label: $label
      ''';
  }
}

class OutputDevice {
  /// The ID used to select the device on the platform.
  final String id;

  /// The label text representation.
  final String label;

  const OutputDevice({
    required this.id,
    required this.label,
  });

  factory OutputDevice.fromMap(Map map) => OutputDevice(
        id: map['id'],
        label: map['label'],
      );

  Map<String, dynamic> toMap() => {
        'id': id,
        'label': label,
      };

  @override
  String toString() {
    return '''
      id: $id
      label: $label
      ''';
  }
}
