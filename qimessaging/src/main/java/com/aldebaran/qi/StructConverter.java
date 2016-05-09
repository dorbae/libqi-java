package com.aldebaran.qi;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Modifier;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Provide methods to automatically convert (recursively) {@link Tuple}s to
 * {@link QiStruct}s.
 */
public class StructConverter
{
  private StructConverter()
  {
    // not instantiable
  }

  public static Object[] tuplesToStructs(Object[] sources, Type[] sourceTypes) throws QiConversionException
  {
    if (sources.length != sourceTypes.length)
      throw new IllegalArgumentException(
          "Sources and sourceTypes length don't match (" + sources.length + " != " + sourceTypes.length + ")");
    Object[] converted = new Object[sources.length];
    for (int i = 0; i < sources.length; ++i)
      converted[i] = tuplesToStructs(sources[i], sourceTypes[i]);
    return converted;
  }

  public static Object tuplesToStructs(Object source, Type targetType) throws QiConversionException
  {
    if (source == null)
      return null;

    if (targetType instanceof Class)
    {
      Class<?> cls = (Class<?>) targetType;
      if (isQiStruct(cls))
        return tupleToStruct((Tuple) source, cls);
    } else if (targetType instanceof ParameterizedType)
    {
      ParameterizedType parameterizedType = (ParameterizedType) targetType;
      Class<?> rawType = (Class<?>) parameterizedType.getRawType();
      if (List.class == rawType)
      {
        List<?> list = (List<?>) source;
        Type itemType = parameterizedType.getActualTypeArguments()[0];
        return tuplesToStructsInList(list, itemType);
      }
      if (Map.class == rawType)
      {
        Map<?, ?> map = (Map<?, ?>) source;
        Type[] types = parameterizedType.getActualTypeArguments();
        Type keyType = types[0];
        Type valueType = types[1];
        return tuplesToStructsInMap(map, keyType, valueType);
      }
    }

    // do not convert
    return source;
  }

  public static <T> T tupleToStruct(Tuple tuple, Class<T> target) throws QiConversionException
  {
    try
    {
      Constructor<T> constructor = target.getDeclaredConstructor();
      constructor.setAccessible(true);
      T struct = constructor.newInstance();
      Field[] fields = target.getDeclaredFields();
      int tupleIndex = 0;
      for (Field field : fields)
      {
        if (tupleIndex >= tuple.size())
          break;
        if (isTransient(field))
          continue;
        Type fieldType = field.getGenericType();
        Object value = tuple.get(tupleIndex++);
        Object convertedValue = tuplesToStructs(value, fieldType);
        field.setAccessible(true);
        field.set(struct, convertedValue);
      }
      return struct;
    } catch (InstantiationException e)
    {
      throw new QiConversionException(e);
    } catch (IllegalAccessException e)
    {
      throw new QiConversionException(e);
    } catch (InvocationTargetException e)
    {
      throw new QiConversionException(e);
    } catch (NoSuchMethodException e)
    {
      throw new QiConversionException(e);
    }
  }

  public static List<?> tuplesToStructsInList(List<?> list, Type itemType) throws QiConversionException
  {
    List<Object> convertedList = new ArrayList<Object>();
    for (Object item : list)
    {
      Object convertedItem = tuplesToStructs(item, itemType);
      convertedList.add(convertedItem);
    }
    return convertedList;
  }

  public static Map<?, ?> tuplesToStructsInMap(Map<?, ?> map, Type keyType, Type valueType) throws QiConversionException
  {
    Map<Object, Object> convertedMap = new HashMap<Object, Object>();
    for (Map.Entry<?, ?> entry : map.entrySet())
    {
      Object convertedKey = tuplesToStructs(entry.getKey(), keyType);
      Object convertedValue = tuplesToStructs(entry.getValue(), valueType);
      convertedMap.put(convertedKey, convertedValue);
    }
    return convertedMap;
  }

  public static Object structsToTuples(Object source) throws QiConversionException
  {
    if (source == null)
      return null;

    if (source instanceof Object[])
      return structsToTuplesInArray((Object[]) source);

    if (source instanceof List)
      return structsToTuplesInList((List<?>) source);

    if (source instanceof Map)
      return structsToTuplesInMap((Map<?, ?>) source);

    if (isQiStruct(source.getClass()))
      return structToTuple(source);

    // do not convert
    return source;
  }

  public static Tuple structToTuple(Object struct) throws QiConversionException
  {
    Class<?> cls = struct.getClass();
    if (!isQiStruct(cls))
      throw new QiConversionException(cls + " has no @QiStruct annotation");

    try
    {
      Field[] fields = cls.getDeclaredFields();
      List<Object> values = new ArrayList<Object>();
      for (Field field : fields) {
        if (isTransient(field))
          continue;
        field.setAccessible(true);
        Object value = field.get(struct);
        Object convertedValue = structsToTuples(value);
        values.add(convertedValue);
      }
      return Tuple.of(values.toArray());
    } catch (IllegalAccessException e)
    {
      throw new QiConversionException(e);
    }
  }

  public static Object[] structsToTuplesInArray(Object[] array) throws QiConversionException
  {
    Object[] convertedArray = new Object[array.length];
    for (int i = 0; i < array.length; ++i)
      convertedArray[i] = structsToTuples(array[i]);
    return convertedArray;
  }

  public static List<?> structsToTuplesInList(List<?> list) throws QiConversionException
  {
    List<Object> convertedList = new ArrayList<Object>();
    for (Object item : list)
    {
      Object convertedItem = structsToTuples(item);
      convertedList.add(convertedItem);
    }
    return convertedList;
  }

  public static Map<?, ?> structsToTuplesInMap(Map<?, ?> map) throws QiConversionException
  {
    Map<Object, Object> convertedMap = new HashMap<Object, Object>();
    for (Map.Entry<?, ?> entry : map.entrySet())
    {
      Object convertedKey = structsToTuples(entry.getKey());
      Object convertedValue = structsToTuples(entry.getValue());
      convertedMap.put(convertedKey, convertedValue);
    }
    return convertedMap;
  }

  private static boolean isQiStruct(Class<?> cls)
  {
    return cls.getAnnotation(QiStruct.class) != null;
  }

  private static boolean isTransient(Field field) {
    return (field.getModifiers() & Modifier.TRANSIENT) != 0;
  }
}
